# Dynarmic Design Documentation

While Dynarmic is a primarily a dynamic recompiler for the ARMv6K architecture, the possibility of supporting 
other versions of the ARM architecture, having a interpreter and/or static recompiler mode, or supporting 
other architectures is kept open. This is done by having each component as modular as possible.
 
Users of this library interact with it primarily through [`src/interface/interface.h`](../src/interface/interface.h). 
Users specify how dynarmic's CPU core interacts with the rest of their systems by setting members of the 
`Dynarmic::UserCallbacks` structure as appropriate. Users setup the CPU state using member fucntions of 
`Dynarmic::Jit`, then call `Dynarmic::Jit::Execute` to start CPU execution. The callbacks defined on `UserCallbacks` 
may be called from dynamically generated code, so users of the library should not depend on the stack being in a 
walkable state for unwinding.

Dynarmic reads instructions from memory by calling `UserCallbacks::MemoryRead32`. These instructions then pass 
through several stages:

1. Decoding (Identifying what type of instruction it is and breaking it up into fields)
2. Translation (Generation of high-level IR from the instruction)
3. Optimization (Eliminiation of redundant microinstructions, other speed improvements)
4. Emission (Generation of host-executable code into memory)
5. Execution (Host CPU jumps to the start of emitted code and runs it)

Using the x64 backend as an example:

* Decoding is done by [double dispatch](https://en.wikipedia.org/wiki/Visitor_pattern) in 
`src/frontend/decoder/{arm.h,thumb16.h,thumb32.h}`.
* Translation is done by the visitors in `src/frontend/translate/{translate_arm.cpp,translate_thumb.cpp}`. 
The function `IR::Block Translate(LocationDescriptor descriptor, MemoryRead32FuncType memory_read_32)` takes a 
memory location and memory reader callback and returns a basic block of IR. 
* The IR can be found under `src/frontend/ir/`.
* Optimization is not implemented yet.
* Emission is done by `EmitX64` which can be found in `src/backend_x64/emit_x64.{h,cpp}`.
* Execution is performed by calling `BlockOfCode::RunCode` in `src/backend_x64/routines.{h,cpp}`.
 
## Decoder

The decoder is a double dispatch decoder. Each instruction is represented by a line in the relevant instruction table.
Here is an example line from `g_arm_instruction_table`:

    INST(&V::arm_ADC_imm,     "ADC (imm)",           "cccc0010101Snnnnddddrrrrvvvvvvvv")
        
(Details on this instruction can be found in section A8.8.1 of the ARMv7-A manual. This is encoding A1.)
    
The first argument to INST is the member function to call on the visitor. The second argument is a user-readable 
instruction name. The third argument is a bit-representation of the instruction.

### Instruction Bit-Representation

Each character in the bitstring represents a bit. A `0` means that that bitposition **must** contain a zero. A `1` 
means that that bitposition **must** contain a one. A `-` means we don't care about the value at that bitposition.
A string of the same character represents a field. In the above example, the first four bits `cccc` represent the 
four-bit-long cond field of the ARM Add with Carry (immediate) instruction.

The visitor would have to have a function named `arm_ADC_imm` with 6 arguments, one for each field (`cccc`, `S`, 
`nnnn`, `dddd`, `rrrr`, `vvvvvvvv`). If there is a mismatch of field number with argument number, a compile-time 
error results.

## Translator

The translator is a visitor that uses the decoder to decode instructions. The translator generates IR code with the 
help of the [`IRBuilder` class](../src/frontend/ir/ir_emitter.h). An example of a translation function follows:

    bool ArmTranslatorVisitor::arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm8 imm8) {
        u32 imm32 = ArmExpandImm(rotate, imm8);
        
        // ADC{S}<c> <Rd>, <Rn>, #<imm>
        
        if (ConditionPassed(cond)) {
            auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.GetCFlag());

            if (d == Reg::PC) {
                ASSERT(!S);
                ir.ALUWritePC(result.result);
                ir.SetTerm(IR::Term::ReturnToDispatch{});
                return false;
            }

            ir.SetRegister(d, result.result);
            if (S) {
                ir.SetNFlag(ir.MostSignificantBit(result.result));
                ir.SetZFlag(ir.IsZero(result.result));
                ir.SetCFlag(result.carry);
                ir.SetVFlag(result.overflow);
            }
        }
        
        return true;
    }

where `ir` is an instance of the `IRBuilder` class. Each member function of the `IRBuilder` class constructs
an IR microinstruction.
 
## Intermediate Representation
 
Dynarmic uses an ordered SSA intermediate representation. It is very vaguely similar to those found in other 
similar projects like redream, nucleus, and xenia. Major differences are: (1) the abundance of context microinstructions 
whereas those projects generally only have two (`load_context`/`store_context`), (2) the explicit handling of 
flags as their own values, and (3) very different basic block edge handling.

The intention of the context microinstructions and explicit flag handling is to allow for future optimizations. The 
differences in the way edges are handled are a quirk of the current implementation and dynarmic will likely add a 
function analyser in the medium-term future.

Dynarmic's intermediate representation is typed. Each microinstruction may take zero or more arguments and may
return zero or more arguments. Each microinstruction is documented below:

### Immediate: Imm{U1,U8,U32,RegRef}

    <u1> ImmU1(u1 value)
    <u8> ImmU8(u8 value)
    <u32> ImmU32(u32 value)
    <RegRef> ImmRegRef(Arm::Reg gpr)

These instructions take a `bool`, `u8` or `u32` value and wraps it up in an IR node so that they can be used
by the IR.

### Context: {Get,Set}Register

    <u32> GetRegister(<RegRef> reg)
    <void> SetRegister(<RegRef> reg, <u32> value)
    
Gets and sets `JitState::Reg[reg]`. Note that `SetRegister(ImmRegRef(Arm::R15), _)` is disallowed by IRBuilder.
Use `{ALU,BX}WritePC` instead.

Note that sequences like `SetRegister(ImmRegRef(Arm::R4), _)` followed by `GetRegister(ImmRegRef(Arm::R4))` ~~are~~ 
*will be* optimized away.
    
### Context: {Get,Set}{N,Z,C,V}Flag

    <u1> GetNFlag()
    <void> SetNFlag(<u1> value)
    <u1> GetZFlag()
    <void> SetZFlag(<u1> value)
    <u1> GetCFlag()
    <void> SetCFlag(<u1> value)
    <u1> GetVFlag()
    <void> SetVFlag(<u1> value)

Gets and sets bits in `JitState::Cpsr`. Similarly to registers redundant get/sets will be optimized away.

### Context: {ALU,BX}WritePC

    <void> ALUWritePC(<u32> value)
    <void> BXWritePC(<u32> value)
    
This should probably be the last instruction in a translation block unless you're doing something fancy.

This microinstruction sets R15 and CPSR.T as appropriate.

### Callback: CallSupervisor

    <void> CallSupervisor(<u32> svc_imm32)

This should probably be the last instruction in a translation block unless you're doing something fancy.

### Calculation: LastSignificant{Half,Byte}

    <u16> LeastSignificantHalf(<u32> value)
    <u8> LeastSignificantByte(<u32> value)

Extract a u16 and u8 respectively from a u32.

### Calculation: MostSignificantBit, IsZero

    <u1> MostSignificantBit(<u32> value)
    <u1> IsZero(<u32> value)
    
These are used to implement ARM flags N and Z. These can often be optimized away by the backend into a host flag read.

### Calculation: LogicalShiftLeft
    
    (<u32> result, <u1> carry_out) LogicalShiftLeft(<u32> operand, <u8> shift_amount, <u1> carry_in)
    
Pseudocode:

        if shift_amount == 0:
            return (operand, carry_in)
            
        x = operand * (2 ** shift_amount)
        result = Bits<31,0>(x)
        carry_out = Bit<32>(x)
        
        return (result, carry_out)
         
This follows ARM semantics. Note `shift_amount` is not masked to 5 bits (like `SHL` does on x64).

### Calculation: LogicalShiftRight

    (<u32> result, <u1> carry_out) LogicalShiftLeft(<u32> operand, <u8> shift_amount, <u1> carry_in)
    
Pseudocode:

        if shift_amount == 0:
            return (operand, carry_in)
            
        x = ZeroExtend(operand, from_size: 32, to_size: shift_amount+32)
        result = Bits<shift_amount+31,shift_amount>(x)
        carry_out = Bit<shift_amount-1>(x)
        
        return (result, carry_out)
             
This follows ARM semantics. Note `shift_amount` is not masked to 5 bits (like `SHR` does on x64).

### Calculation: ArithmeticShiftRight

    (<u32> result, <u1> carry_out) ArithmeticShiftRight(<u32> operand, <u8> shift_amount, <u1> carry_in)
    
Pseudocode:

        if shift_amount == 0:
            return (operand, carry_in)
            
        x = SignExtend(operand, from_size: 32, to_size: shift_amount+32)
        result = Bits<shift_amount+31,shift_amount>(x)
        carry_out = Bit<shift_amount-1>(x)
        
        return (result, carry_out)
             
This follows ARM semantics. Note `shift_amount` is not masked to 5 bits (like `SAR` does on x64).

### Calcuation: RotateRight

    (<u32> result, <u1> carry_out) RotateRight(<u32> operand, <u8> shift_amount, <u1> carry_in)
    
Pseudocode:

        if shift_amount == 0:
            return (operand, carry_in)
            
        shift_amount %= 32
        result = (operand << shift_amount) | (operand >> (32 - shift_amount))
        carry_out = Bit<31>(result)
        
        return (result, carry_out)

### Calculation: AddWithCarry

    (<u32> result, <u1> carry_out, <u1> overflow) AddWithCarry(<u32> a, <u32> b, <u1> carry_in)

a + b + carry_in

### Calculation: SubWithCarry

    (<u32> result, <u1> carry_out, <u1> overflow) SubWithCarry(<u32> a, <u32> b, <u1> carry_in)
    
This has equivalent semantics to `AddWithCarry(a, Not(b), carry_in)`.

a - b - !carry_in

### Calculation: And

    <u32> And(<u32> a, <u32> b)
    
### Calculation: Eor

    <u32> Eor(<u32> a, <u32> b)
    
Exclusive OR (i.e.: XOR)

### Calculation: Or

    <u32> Or(<u32> a, <u32> b)
    
### Calculation: Not

    <u32> Not(<u32> value)

### Callback: {Read,Write}Memory{8,16,32,64}

    <u8> ReadMemory8(<u32> vaddr)
    <u8> ReadMemory16(<u32> vaddr)
    <u8> ReadMemory32(<u32> vaddr)
    <u8> ReadMemory64(<u32> vaddr)
    <void> WriteMemory8(<u32> vaddr, <u8> value_to_store)
    <void> WriteMemory16(<u32> vaddr, <u16> value_to_store)
    <void> WriteMemory32(<u32> vaddr, <u32> value_to_store)
    <void> WriteMemory64(<u32> vaddr, <u64> value_to_store)

Memory access.

### Terminal: Interpret

    SetTerm(IR::Term::Interpret{next})
                
This terminal instruction calls the interpreter, starting at `next`.
The interpreter must interpret ~~at least 1 instruction but may choose to interpret more.~~
**exactly one instruction (in the current implementation).**
              
### Terminal: ReturnToDispatch

    SetTerm(IR::Term::ReturnToDispatch{})             
              
This terminal instruction returns control to the dispatcher.
The dispatcher will use the value in R15 to determine what comes next.
              
### Terminal: LinkBlock

    SetTerm(IR::Term::LinkBlock{next})

This terminal instruction jumps to the basic block described by `next` if we have enough
cycles remaining. If we do not have enough cycles remaining, we return to the
dispatcher, which will return control to the host.

### Terminal: LinkBlockFast

    SetTerm(IR::Term::LinkBlockFast{next})

This terminal instruction jumps to the basic block described by `next` unconditionally.
This is an optimization and MUST only be emitted when this is guaranteed not to result
in hanging, even in the face of other optimizations. (In practice, this means that only
forward jumps to short-ish blocks would use this instruction.)
A backend that doesn't support this optimization may choose to implement this exactly
as LinkBlock.

**(degasus says this is probably a pretty useless optimization)**

### Terminal: PopRSBHint

    SetTerm(IR::Term::PopRSBHint{})

This terminal instruction checks the top of the Return Stack Buffer against R15.
If RSB lookup fails, control is returned to the dispatcher.
This is an optimization for faster function calls. A backend that doesn't support
this optimization or doesn't have a RSB may choose to implement this exactly as
ReturnToDispatch.

**(This would be quite profitable once implemented. degasus agrees.)**

### Terminal: If

    SetTerm(IR::Term::If{cond, term_then, term_else})

~~This terminal instruction conditionally executes one terminal or another depending 
on the run-time state of the ARM flags.~~
              
**(Unimplemented.)**
