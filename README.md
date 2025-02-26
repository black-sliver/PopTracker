# PopTracker Pack List

List of packs and their update URLs provided by the community.

Send PRs for `community-packs.json` on GitHub to add your pack or remove a pack with wrong or outdated authorship.
You can use the pencil button to create a PR directly on GitHub.

## packs.json

The list of uid -> pack info. Links should point directly to the file/download.

See [examples/packs.json](examples/packs.json)
and [schema/packs.schema.json](schema/packs.schema.json) for examples and details.

## versions.json

`packs.json` links to a `versions.json` for each pack.
It lists the available versions for update and changelog and has to be transferred over https (not http).

The top entry in the list is considered the latest version.

See [examples/versions.json](examples/versions.json)
and [schema/versions.schema.json](schema/versions.schema.json) for examples and details.

### sha256

`versions.json` needs a checksum for each zip

**Windows**
* use 7zip, right click on pack.zip -> CRC SHA -> SHA-256
* or run `CertUtil -hashfile path\to\pack.zip SHA256` and remove all spaces
* or run `powershell Get-FileHash -Algorithm SHA256 path\to\pack.zip | Format-List`

**Linux**
* `sha256sum path/to/pack.zip`

**MacOS**
* `openssl dgst -sha256 path/to/pack.zip`
