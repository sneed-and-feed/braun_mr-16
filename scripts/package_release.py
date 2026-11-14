import os
import sys
import glob
import ctypes
import zipfile
import json
import hashlib

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
release_dir = os.path.join(root_dir, "releases")
build_dir = os.path.join(root_dir, "build", "BRAUN_MR16_artefacts", "Release")
if not os.path.exists(build_dir):
    build_dir = os.path.join(root_dir, "build", "BRAUN_MR16_artefacts")

os.makedirs(release_dir, exist_ok=True)

# Extract version from command line or package.json
if len(sys.argv) > 1 and sys.argv[1].strip():
    version = sys.argv[1].strip()
else:
    with open(os.path.join(root_dir, "package.json"), "r", encoding="utf-8") as f:
        pkg = json.load(f)
    version = pkg.get("version", "1.1.0")


def verify_windows_pe_version(pe_path: str, expected_version: str, expected_company: str) -> None:
    if os.name != "nt" or not os.path.exists(pe_path):
        return
    size = ctypes.windll.version.GetFileVersionInfoSizeW(pe_path, None)
    if not size:
        raise RuntimeError(f"Missing Windows VERSIONINFO resource in {pe_path}")
    buf = ctypes.create_string_buffer(size)
    if not ctypes.windll.version.GetFileVersionInfoW(pe_path, 0, size, buf):
        raise RuntimeError(f"Failed to read Windows VERSIONINFO from {pe_path}")

    def query_str(field: str) -> str:
        ptr = ctypes.c_void_p()
        length = ctypes.c_uint()
        sub_block = f"\\StringFileInfo\\040904E4\\{field}"
        if not ctypes.windll.version.VerQueryValueW(buf, sub_block, ctypes.byref(ptr), ctypes.byref(length)):
            return ""
        return ctypes.wstring_at(ptr.value).strip()

    file_ver = query_str("FileVersion")
    prod_ver = query_str("ProductVersion")
    company = query_str("CompanyName")
    if file_ver != expected_version or prod_ver != expected_version:
        raise RuntimeError(
            f"PE VERSIONINFO mismatch in {pe_path}: FileVersion={file_ver!r}, "
            f"ProductVersion={prod_ver!r}, expected {expected_version!r}"
        )
    if company != expected_company:
        raise RuntimeError(
            f"PE CompanyName mismatch in {pe_path}: CompanyName={company!r}, expected {expected_company!r}"
        )


expected_company = "Sneed's Feed & Seed Ltd."
vst3_pe = os.path.join(build_dir, "VST3", "BRAUN_MR16.vst3", "Contents", "x86_64-win", "BRAUN_MR16.vst3")
standalone_pe = os.path.join(build_dir, "Standalone", "BRAUN_MR16.exe")
verify_windows_pe_version(vst3_pe, version, expected_company)
verify_windows_pe_version(standalone_pe, version, expected_company)

# Remove stale release archives from older versions so releases/ stays clean
for existing_zip in glob.glob(os.path.join(release_dir, "BRAUN_MR16-v*.zip")):
    base = os.path.basename(existing_zip)
    if not base.startswith(f"BRAUN_MR16-v{version}-"):
        os.remove(existing_zip)
        print(f"Removed stale archive: {existing_zip}")

install_guide = f"""BRAUN MR-16 v{version} — MODAL RESONATOR & KINETIC SYNTHESIZER
Standard: DIN 1451 Technical Specification

LEGAL NOTICE:
Not affiliated with Braun GmbH. Dieter Rams inspired design homage.
Manufacturer: Sneed's Feed & Seed Ltd.

INSTALLATION GUIDE:
1. VST3: Copy directory 'BRAUN_MR16.vst3' to %CommonProgramFiles%\\VST3\\
   (Default: C:\\Program Files\\Common Files\\VST3\\BRAUN_MR16.vst3)
2. CLAP: Copy 'BRAUN_MR16.clap' to %CommonProgramFiles%\\CLAP\\
   (Default: C:\\Program Files\\Common Files\\CLAP\\BRAUN_MR16.clap)
3. Standalone: Launch 'BRAUN_MR16.exe' directly.
"""

doc_files = ["README.txt", "README.md", "ARCHITECTURE.md", "CHANGELOG.md", "LICENSE"]

# 1. Package Windows-x64 full zip
zip_path = os.path.join(release_dir, f"BRAUN_MR16-v{version}-Windows-x64.zip")
with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
    vst3_dir = os.path.join(build_dir, "VST3", "BRAUN_MR16.vst3")
    if os.path.exists(vst3_dir):
        for root, dirs, files in os.walk(vst3_dir):
            for f in files:
                full = os.path.join(root, f)
                rel = os.path.relpath(full, os.path.join(build_dir, "VST3"))
                zf.write(full, rel)
    clap_file = os.path.join(build_dir, "CLAP", "BRAUN_MR16.clap")
    if os.path.exists(clap_file):
        zf.write(clap_file, "BRAUN_MR16.clap")
    standalone_file = os.path.join(build_dir, "Standalone", "BRAUN_MR16.exe")
    if os.path.exists(standalone_file):
        zf.write(standalone_file, "BRAUN_MR16.exe")
    zf.writestr("INSTALL.txt", install_guide)
    for doc in doc_files:
        doc_path = os.path.join(root_dir, doc)
        if os.path.exists(doc_path):
            zf.write(doc_path, doc)

print(f"Created {zip_path}: {os.path.getsize(zip_path):,} bytes")

# 2. Package VST3-only zip
vst3_zip_path = os.path.join(release_dir, f"BRAUN_MR16-v{version}-VST3-Windows-x64.zip")
with zipfile.ZipFile(vst3_zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
    vst3_dir = os.path.join(build_dir, "VST3", "BRAUN_MR16.vst3")
    if os.path.exists(vst3_dir):
        for root, dirs, files in os.walk(vst3_dir):
            for f in files:
                full = os.path.join(root, f)
                rel = os.path.relpath(full, os.path.join(build_dir, "VST3"))
                zf.write(full, rel)
    zf.writestr("INSTALL.txt", install_guide)
    for doc in doc_files:
        doc_path = os.path.join(root_dir, doc)
        if os.path.exists(doc_path):
            zf.write(doc_path, doc)

print(f"Created {vst3_zip_path}: {os.path.getsize(vst3_zip_path):,} bytes")

# 3. Calculate SHA-256 checksums for active v{version} archives ONLY and write releases/SHA256SUMS.txt
active_zip_names = sorted([
    os.path.basename(zip_path),
    os.path.basename(vst3_zip_path),
])
sha256_lines = []
for zf_name in active_zip_names:
    zf_full = os.path.join(release_dir, zf_name)
    if os.path.exists(zf_full):
        h = hashlib.sha256()
        with open(zf_full, "rb") as bf:
            while chunk := bf.read(65536):
                h.update(chunk)
        sha256_lines.append(f"{h.hexdigest()}  {zf_name}\n")

checksum_file = os.path.join(release_dir, "SHA256SUMS.txt")
with open(checksum_file, "w", encoding="utf-8") as f:
    f.writelines(sha256_lines)

print(f"Wrote cryptographic checksums to {checksum_file}")
