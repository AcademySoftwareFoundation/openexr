version = ""
with open('src/lib/OpenEXRCore/openexr_version.h', 'r') as f:
    txt = f.read()
    for name in ('MAJOR', 'MINOR', 'PATCH'):
        prefix = 'set(OPENEXR_{}_VERSION '.format(name)
        subtxt = txt[txt.find(prefix) + len(prefix):]
        subtxt = subtxt[:subtxt.find(')\n')]
        version = version + subtxt + '.'
    version = version[:-1]


with open('pyproject.toml', 'r') as f:
    txt = f.read()

txt = txt.replace(r"${OPENEXR_VERSION}", version)

with open('pyproject.toml', 'w') as f:
    f.write(txt)
