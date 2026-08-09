const catalogUrl = "versions.json";
const installButton = document.querySelector("#install-button");
const versionSelect = document.querySelector("#firmware-version");
const variantSelect = document.querySelector("#firmware-variant");
const variantNote = document.querySelector("#variant-note");
const versionBadge = document.querySelector(".version-badge");
const versionNote = document.querySelector("#version-note");
const introCopy = document.querySelector(".intro-copy");

let firmwareVersions = [];
const manifestObjectUrls = [];

function getVariants(version) {
  return version.variants && version.variants.length > 0
    ? version.variants
    : [
        {
          id: "standard",
          label: "Standard",
          description: "Standard-Firmware ohne MCP4725-DAC.",
          firmware: version.firmware,
        },
      ];
}

function createManifest(version, variant) {
  const firmwareUrl = new URL(variant.firmware, document.baseURI).href;
  const manifest = {
    name: "Aiuto-Vista",
    version: version.version,
    new_install_prompt_erase: true,
    new_install_improv_wait_time: 0,
    builds: [
      {
        chipFamily: "ESP32",
        improv: false,
        parts: [{ path: firmwareUrl, offset: 0 }],
      },
    ],
  };

  return URL.createObjectURL(
    new Blob([JSON.stringify(manifest)], { type: "application/json" }),
  );
}

function populateVariantSelect(version) {
  const variants = getVariants(version);
  variantSelect.replaceChildren();

  for (const variant of variants) {
    const option = document.createElement("option");
    option.value = variant.id;
    option.textContent = variant.label;
    variantSelect.append(option);
  }
}

function selectVersion(versionNumber) {
  const version = firmwareVersions.find(
    (candidate) => candidate.version === versionNumber,
  );
  if (!version) {
    return;
  }

  populateVariantSelect(version);
  variantSelect.value = getVariants(version)[0].id;
  variantSelect.disabled = getVariants(version).length < 2;
  selectVariant();
}

function selectVariant() {
  const version = firmwareVersions.find(
    (candidate) => candidate.version === versionSelect.value,
  );
  if (!version) {
    return;
  }

  const variant = getVariants(version).find(
    (candidate) => candidate.id === variantSelect.value,
  );
  if (!variant) {
    return;
  }

  const manifestObjectUrl = createManifest(version, variant);
  manifestObjectUrls.push(manifestObjectUrl);
  installButton.setAttribute("manifest", manifestObjectUrl);

  versionBadge.lastChild.textContent = ` Firmware ${version.version}`;
  variantNote.textContent = variant.description;
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
      "Der Versionskatalog ist nicht erreichbar. Version 0.7.0 bleibt als Standard ausgewählt.";
  }
}

versionSelect.addEventListener("change", () => {
  selectVersion(versionSelect.value);
});

variantSelect.addEventListener("change", () => {
  selectVariant();
});

window.addEventListener("pagehide", () => {
  for (const manifestObjectUrl of manifestObjectUrls) {
    URL.revokeObjectURL(manifestObjectUrl);
  }
});

loadVersionCatalog();