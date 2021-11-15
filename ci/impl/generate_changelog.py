import os
import sys
import semantic_version
from utils import find_skymp_root
from utils import get_sp_version
from utils import set_sp_version

def main(argv):
    generate_changelog()

def generate_changelog():
    old_version_full = get_sp_version()
    new_version_full = str(semantic_version.Version(old_version_full).next_minor())
    set_sp_version(new_version_full)
    
    # For printing, omit fix if 0
    old_version = old_version_full[:-2] if old_version_full.endswith(".0") else old_version_full
    new_version = new_version_full[:-2] if new_version_full.endswith(".0") else new_version_full

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
    changelog += "This document includes changes made since SP " + old_version + ".\n\n"
    changelog += "SP updates regularly. This update probably doesn't include ALL patches that have to be made.\n"
    changelog += "There are still many things to be implemented or fixed. See [issues](https://github.com/skyrim-multiplayer/skymp/issues?q=is%3Aopen+is%3Aissue+label%3Aarea%3Askyrim-platform).\n\n"
    changelog += "Please note that the current SP version only works for the old SE build (before the 11.11.21 update).\n"
    changelog += "To downgrade your Skyrim SE installation use [this patch](https://www.nexusmods.com/skyrimspecialedition/mods/57618)."

    for entry in changelog_entries_normal:
        # New line if not start of changelog
        changelog += "\n\n" if len(changelog) > 0 else ""
        
        changelog += "\n".join(entry.splitlines())

    changelog += "\n\n" + "## Other changes" "\n\n"

    for entry in changelog_entries_other:
        changelog += "- " + entry

    changelog_out_path = os.path.join(find_skymp_root(), "docs", "release", "sp-" + new_version + ".md")
    with open(changelog_out_path, "w") as text_file:
        text_file.write(changelog)

if __name__ == "__main__":
    main(sys.argv[1:])
