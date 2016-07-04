Dynarmic
========

An (eventual) dynamic recompiler for ARMv6K. The code is a mess.

A lot of optimization work can be done (it currently produces bad code, worse than that non-IR JIT).

Plans
-----

### Near-term

* Actually finish the translators off
* Get everything working
* Redundant Get/Set elimination
* Handle immediates properly
* Allow ARM flags to be stored in host flags

### Medium-term

* [Optimizations](https://gist.github.com/MerryMage/a7411f139cc6e506ea7689ff4579c84b)

### Long-term

* ARMv7A support
* ARMv5 support
