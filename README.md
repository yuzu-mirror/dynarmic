Dynarmic
========

An (eventual) dynamic recompiler for ARM6K. The code is a mess.

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

* Undo strength-reduction (invert some common ARM idioms)
* Other optimizations

### Long-term

* ARMv7A support
* ARMv5 support
