#!/usr/bin/env bash
# Usage: ./release.sh <version>
# Updates the version number everywhere it appears, commits, and creates tag v<version>.
set -euo pipefail

cd "$(dirname "$0")"

if [[ $# -ne 1 || ! $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "usage: $0 <major.minor.patch>" >&2
    exit 1
fi
version=$1
tag="v$version"

if [[ -n $(git status --porcelain --untracked-files=no) ]]; then
    echo "error: working tree has uncommitted changes, commit or stash them first" >&2
    exit 1
fi
if git rev-parse "$tag" >/dev/null 2>&1; then
    echo "error: tag $tag already exists" >&2
    exit 1
fi

sed -i -E "s/^(project\(kwin-ease-cursor-crossing VERSION )[0-9.]+\)/\1$version)/" CMakeLists.txt
sed -i -E "s/^pkgver=.*/pkgver=$version/; s/^pkgrel=.*/pkgrel=1/" dist/arch/PKGBUILD
sed -i -E "s/^(  version = \")[0-9.]+(\";)/\1$version\2/" dist/nixos/package.nix

# verify every file now carries the new version
grep -q "VERSION $version)" CMakeLists.txt
grep -q "^pkgver=$version$" dist/arch/PKGBUILD
grep -q "version = \"$version\";" dist/nixos/package.nix

if ! git diff --quiet; then
    git add CMakeLists.txt dist/arch/PKGBUILD dist/nixos/package.nix
    git commit -m "release $version"
fi
git tag -a "$tag" -m "Release $version"

echo "tagged $tag"
echo "publish with: git push && git push origin $tag"
