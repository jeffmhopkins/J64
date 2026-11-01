# Releasing

> Every push and pull request builds every ROM. A version tag builds them again and
> publishes a GitHub Release with the `.z64` files attached.

<a id="ci"></a>
## Every push and PR

[.github/workflows/build.yml](../.github/workflows/build.yml) builds the dev image (cached
between runs), runs the host tests, builds every ROM, and uploads `dist/*.z64` as an
artifact named `j64-roms-<commit>`. Download it from the run's **Summary** page to try a
build without compiling it yourself.

<a id="release"></a>
## Cutting a release

```sh
git checkout main && git pull
git tag v0.1.0
git push origin v0.1.0
```

[.github/workflows/release.yml](../.github/workflows/release.yml) then builds with
`J64_VERSION` set to the tag, renames each ROM to `<name>-v0.1.0.z64`, and creates the
release with notes generated from the merged PRs.

> [!NOTE]
> Tags with a hyphen, such as `v0.2.0-beta.1`, are published as pre-releases.

> [!WARNING]
> Tag only commits on `main` that CI has already built green. A release can't be rebuilt
> from the same tag without deleting and re-pushing the tag.
