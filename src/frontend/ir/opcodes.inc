//     opcode name,             return type,    arg1 type,      arg2 type,      arg3 type,      ...

// Immediate values
OPCODE(ImmU1,                   T::U1,                                                          )
OPCODE(ImmU8,                   T::U8,                                                          )
OPCODE(ImmU32,                  T::U32,                                                         )
OPCODE(ImmRegRef,               T::RegRef,                                                      )

// ARM Context getters/setters
OPCODE(GetRegister,             T::U32,         T::RegRef                                       )
OPCODE(SetRegister,             T::Void,        T::RegRef,      T::U32                          )
OPCODE(GetNFlag,                T::U1,                                                          )
OPCODE(SetNFlag,                T::Void,        T::U1                                           )
OPCODE(GetZFlag,                T::U1,                                                          )
OPCODE(SetZFlag,                T::Void,        T::U1                                           )
OPCODE(GetCFlag,                T::U1,                                                          )
OPCODE(SetCFlag,                T::Void,        T::U1                                           )
OPCODE(GetVFlag,                T::U1,                                                          )
OPCODE(SetVFlag,                T::Void,        T::U1                                           )

// Pseudo-operation, handled specially at final emit
OPCODE(GetCarryFromOp,          T::U1,          T::U32                                          )
OPCODE(GetOverflowFromOp,       T::U1,          T::U32                                          )

// Calculations
OPCODE(LeastSignificantByte,    T::U8,          T::U32                                          )
OPCODE(MostSignificantBit,      T::U1,          T::U32                                          )
OPCODE(IsZero,                  T::U1,          T::U32                                          )
OPCODE(LogicalShiftLeft,        T::U32,         T::U32,         T::U8,          T::U1           )
OPCODE(LogicalShiftRight,       T::U32,         T::U32,         T::U8,          T::U1           )
OPCODE(ArithmeticShiftRight,    T::U32,         T::U32,         T::U8,          T::U1           )
OPCODE(AddWithCarry,            T::U32,         T::U32,         T::U32,         T::U1           )
OPCODE(And,                     T::U32,         T::U32,         T::U32                          )
OPCODE(Eor,                     T::U32,         T::U32,         T::U32                          )
