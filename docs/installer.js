const catalogUrl = "versions.json";
const installButton = document.querySelector("#install-button");
const versionSelect = document.querySelector("#firmware-version");
const versionBadge = document.querySelector(".version-badge");
const versionNote = document.querySelector("#version-note");
const introCopy = document.querySelector(".intro-copy");

let firmwareVersions = [];
const manifestObjectUrls = [];

function createManifest(version) {
  const firmwareUrl = new URL(version.firmware, document.baseURI).href;
  const manifest = {
    name: "ScopeBuddy",
    version: version.version,
    new_install_prompt_erase: true,
    new_install_improv_wait_time: 0,
    builds: [
      {
        chipFamily: "ESP32-P4",
        improv: false,
        parts: [{ path: firmwareUrl, offset: 0 }],
      },
    ],
  };

  return URL.createObjectURL(
    new Blob([JSON.stringify(manifest)], { type: "application/json" }),
  );
}

function selectVersion(versionNumber) {
  const version = firmwareVersions.find(
    (candidate) => candidate.version === versionNumber,
  );
  if (!version) {
    return;
  }

  const manifestObjectUrl = createManifest(version);
  manifestObjectUrls.push(manifestObjectUrl);
  installButton.setAttribute("manifest", manifestObjectUrl);

  versionBadge.lastChild.textContent = ` Firmware ${version.version}`;
  introCopy.textContent =
    `Diese Webseite überträgt ScopeBuddy ${version.version} direkt über USB ` +
    "auf das Display. Dafür sind weder ESP-IDF noch Python oder Git notwendig.";
  versionNote.textContent = version.recommended
    ? `Version ${version.version} ist die aktuelle, empfohlene Version.`
    : `Version ${version.version} ist eine ältere Firmware-Version. Beim Downgrade werden vorhandene Einstellungen gelöscht.`;
}

function populateVersionSelect(catalog) {
  firmwareVersions = catalog.versions;
  versionSelect.replaceChildren();

  for (const version of firmwareVersions) {
    const option = document.createElement("option");
    option.value = version.version;
    option.textContent = version.recommended
      ? `${version.version} (aktuell)`
      : version.version;
    versionSelect.append(option);
  }

  versionSelect.value = catalog.current;
  versionSelect.disabled = firmwareVersions.length < 2;
  selectVersion(catalog.current);
}

async function loadVersionCatalog() {
  try {
    const response = await fetch(catalogUrl, { cache: "no-cache" });
    if (!response.ok) {
      throw new Error(`Versionskatalog konnte nicht geladen werden (${response.status})`);
    }

    const catalog = await response.json();
    if (
      !catalog.current ||
      !Array.isArray(catalog.versions) ||
      catalog.versions.length === 0
    ) {
      throw new Error("Versionskatalog ist ungültig");
    }
    populateVersionSelect(catalog);
  } catch (error) {
    console.error(error);
    versionSelect.disabled = true;
    versionNote.textContent =
      "Der Versionskatalog ist nicht erreichbar. Version 0.5.1 bleibt als Standard ausgewählt.";
  }
}

versionSelect.addEventListener("change", () => {
  selectVersion(versionSelect.value);
});

window.addEventListener("pagehide", () => {
  for (const manifestObjectUrl of manifestObjectUrls) {
    URL.revokeObjectURL(manifestObjectUrl);
  }
});

loadVersionCatalog();
