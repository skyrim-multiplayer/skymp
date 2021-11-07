import os
from utils import find_skymp_root

old_version = "2.0"
new_version = "2.1"

changelog_entries = []

directory = os.path.join(find_skymp_root(), "docs", "release", "dev")
for filename in os.listdir(directory):
    if filename.endswith(".md") and not filename in ("0-sample-topic.md", "changelog.md"):
        path = os.path.join(directory, filename)
        with open(path) as f:
            s = f.read()
            changelog_entries.insert(-1, s)
            os.remove(path)
    else:
        continue

changelog_entries_normal = []
changelog_entries_other = []

for entry in changelog_entries:
    goes_to_other = entry.find("#") == -1
    if goes_to_other:
        changelog_entries_other.insert(-1, entry)
    else:
        changelog_entries_normal.insert(-1, entry)

changelog_entries_normal = reversed(sorted(changelog_entries_normal, key=len))

changelog = ""
changelog += "# SP " + new_version + " Release Notes\n\n"
changelog += "Changes made since SP " + old_version + " include the following.\n"

for entry in changelog_entries_normal:
    # New line if not start of changelog
    changelog += "\n" if len(changelog) > 0 else ""

    # Write all lines except the first (with # sign)
    changelog += "\n".join(entry.splitlines())

changelog += "\n" + "## Other changes" "\n\n"

for entry in changelog_entries_other:
    changelog += "- " + entry

changelog_out_path = os.path.join(find_skymp_root(), "docs", "release", "sp-" + new_version + ".md")
with open(changelog_out_path, "w") as text_file:
    text_file.write(changelog)
