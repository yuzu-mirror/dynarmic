# Register Allocation (x64 Backend)

`HostLoc`s contain values. A `HostLoc` ("host value location") is either a host CPU register or a host spill location.

Values once set cannot be changed. Values can however be moved by the register allocator between `HostLoc`s. This is handled by the register allocator itself and code that uses the register allocator need not and should not move values between registers.

The register allocator is based on three concepts: `Use`, `Def` and `Scratch`.

* `Use`: The use of a value.
* `Def`: The definition of a value, this is the only time when a value is set.
* `Scratch`: Allocate a register that can be freely modified as one wishes.

Note that `Use`ing a value decrements its `use_count` by one. When the `use_count` reaches zero the value is discarded and no longer exists.

The member functions on `RegAlloc` are just a combination of the above concepts.

### `Scratch`

    Xbyak::Reg64 ScratchGpr(HostLocList desired_locations = any_gpr)
    Xbyak::Xmm ScratchXmm(HostLocList desired_locations = any_xmm)

At runtime, allocate one of the registers in `desired_locations`. You are free to modify the register. The register is discarded at the end of the allocation scope.

### Pure `Use`

    Xbyak::Reg64 UseGpr(IR::Value use_value, HostLocList desired_locations = any_gpr);
    Xbyak::Xmm UseXmm(IR::Value use_value, HostLocList desired_locations = any_xmm);
    OpArg UseOpArg(IR::Value use_value, HostLocList desired_locations);

At runtime, the value corresponding to `use_value` will be placed into one of the `HostLoc`s specified by `desired_locations`. The return value is the actual location.

This register **must not** have it's value changed.

* `UseGpr`: The location is a GPR.
* `UseXmm`: The location is an XMM register.
* `UseOpArg`: The location may be one of the locations specified by `desired_locations`, but may also be a host memory reference.

### `UseScratch`

    Xbyak::Reg64 UseScratchGpr(IR::Value use_value, HostLocList desired_locations = any_gpr)
    Xbyak::Xmm UseScratchXmm(IR::Value use_value, HostLocList desired_locations = any_xmm)

At runtime, the value corresponding to `use_value` will be placed into one of the `HostLoc`s specified by `desired_locations`. The return value is the actual location.

You are free to modify the register. The register is discarded at the end of the allocation scope.

### `Def`

A `Def` is the defintion of a value. This is the only time when a value may be set.

    Xbyak::Xmm DefXmm(IR::Inst* def_inst, HostLocList desired_locations = any_xmm)
    Xbyak::Reg64 DefGpr(IR::Inst* def_inst, HostLocList desired_locations = any_gpr)

By calling `DefXmm` or `DefGpr`, you are stating that you wish to define the value for `def_inst`, and you wish to write the value to one of the `HostLoc`s specified by `desired_locations`. You must write the value to the register returned.

### `AddDef`

Adding a `Def` to an existing value.

    void RegisterAddDef(IR::Inst* def_inst, const IR::Value& use_inst);

You are declaring that the value for `def_inst` is the same as the value for `use_inst`. No host machine instructions are emitted.

### `UseDef`

    Xbyak::Reg64 UseDefGpr(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_gpr)
    Xbyak::Xmm UseDefXmm(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_xmm)

At runtime, the value corresponding to `use_value` will be placed into one of the `HostLoc`s specified by `desired_locations`. The return value is the actual location. You must write the value correponding to `def_inst` by the end of the allocation scope.

### `UseDef` (OpArg variant)

    std::tuple<OpArg, Xbyak::Reg64> UseDefOpArgGpr(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_gpr)
    std::tuple<OpArg, Xbyak::Xmm> UseDefOpArgXmm(IR::Value use_value, IR::Inst* def_inst, HostLocList desired_locations = any_xmm)

These have the same semantics as `UseDefGpr` and `UseDefXmm` except `use_value` may not be present in the register, and may actually be in a host memory location.

## When to use each?

The variety of different ways to `Use` and `Def` values are for performance reasons.

* `UseDef`: Instead of performing a `Use` and a `Def`, `UseDef` uses one less register in the case when this `Use` is the last `Use` of a value.
* `UseScratch`: Instead of performing a `Use` and a `Scratch`, `UseScratch` uses one less register in the case when this `Use` is the last `Use` of a value.
* `AddDef`: This drastically reduces the number of registers required when it can be used. It can be used when values are truncations of other values. For example, if `u8_value` contains the truncation of `u32_value`, `AddDef(u8_value, u32_value)` is a valid definition of `u8_value`.
* OpArg variants: Save host code-cache by merging memory loads into other instructions instead of the register allocator having to emit a `mov`.