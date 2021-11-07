import os

changelog_entries = []

directory = "."
for filename in os.listdir(directory):
    if filename.endswith(".md") and not filename in ("0-sample-topic.md", "changelog.md"):
        path = os.path.join(directory, filename)
        with open(path) as f:
            s = f.read()
            changelog_entries.insert(-1, s)
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

for entry in changelog_entries_normal:
    # New line if not start of changelog
    changelog += "\n" if len(changelog) > 0 else ""

    # Write all lines except the first (with # sign)
    changelog += "\n".join(entry.splitlines())

changelog += "\n" + "## Other changes" "\n\n"

for entry in changelog_entries_other:
    changelog += "- " + entry

with open("changelog.md", "w") as text_file:
    text_file.write(changelog)
