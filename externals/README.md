This repository uses subtrees to manage some of its externals.

## Initial setup

```
git remote add externals-fmt https://github.com/fmtlib/fmt.git --no-tags
git remote add externals-mp https://github.com/MerryMage/mp.git --no-tags
git remote add externals-robin-map https://github.com/Tessil/robin-map.git --no-tags
git remote add externals-vixl https://git.linaro.org/arm/vixl.git --no-tags
git remote add externals-xbyak https://github.com/herumi/xbyak.git --no-tags
```

## Updating

Change `<ref>` to refer to the appropriate git reference.

```
git fetch externals-fmt
git fetch externals-mp
git fetch externals-robin-map
git fetch externals-vixl
git fetch externals-xbyak
git subtree pull --squash --prefix=externals/fmt externals-fmt <ref>
git subtree pull --squash --prefix=externals/mp externals-mp <ref>
git subtree pull --squash --prefix=externals/robin-map externals-robin-map <ref>
git subtree pull --squash --prefix=externals/vixl/vixl externals-vixl <ref>
git subtree pull --squash --prefix=externals/xbyak externals-xbyak <ref>
```
