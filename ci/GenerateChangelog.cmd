cls
pip install semantic_version
python impl/generate_changelog.py
if NOT ["%errorlevel%"]==["0"] pause
