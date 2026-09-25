import os
import zipfile
import json
import hashlib

root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
release_dir = os.path.join(root_dir, "releases")
build_dir = os.path.join(root_dir, "build", "BRAUN_MR16_artefacts", "Release")
if not os.path.exists(build_dir):
    build_dir = os.path.join(root_dir, "build", "BRAUN_MR16_artefacts")

os.makedirs(release_dir, exist_ok=True)

# Extract version from package.json
with open(os.path.join(root_dir, "package.json"), "r", encoding="utf-8") as f:
    pkg = json.load(f)
version = pkg.get("version", "1.0.12")

install_guide = """BRAUN MR-16 MODAL RESONATOR & KINETIC SYNTHESIZER
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
    if os.path.exists(os.path.join(root_dir, "README.txt")):
        zf.write(os.path.join(root_dir, "README.txt"), "README.txt")
    if os.path.exists(os.path.join(root_dir, "README.md")):
        zf.write(os.path.join(root_dir, "README.md"), "README.md")
    if os.path.exists(os.path.join(root_dir, "LICENSE")):
        zf.write(os.path.join(root_dir, "LICENSE"), "LICENSE")

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
    if os.path.exists(os.path.join(root_dir, "README.txt")):
        zf.write(os.path.join(root_dir, "README.txt"), "README.txt")
    if os.path.exists(os.path.join(root_dir, "README.md")):
        zf.write(os.path.join(root_dir, "README.md"), "README.md")
    if os.path.exists(os.path.join(root_dir, "LICENSE")):
        zf.write(os.path.join(root_dir, "LICENSE"), "LICENSE")

print(f"Created {vst3_zip_path}: {os.path.getsize(vst3_zip_path):,} bytes")

# 3. Calculate SHA-256 checksums and write releases/SHA256SUMS.txt
sha256_lines = []
for zf_name in sorted(os.listdir(release_dir)):
    if zf_name.endswith(".zip"):
        zf_full = os.path.join(release_dir, zf_name)
        h = hashlib.sha256()
        with open(zf_full, "rb") as bf:
            while chunk := bf.read(65536):
                h.update(chunk)
        sha256_lines.append(f"{h.hexdigest()}  {zf_name}\n")

checksum_file = os.path.join(release_dir, "SHA256SUMS.txt")
with open(checksum_file, "w", encoding="utf-8") as f:
    f.writelines(sha256_lines)

print(f"Wrote cryptographic checksums to {checksum_file}")
