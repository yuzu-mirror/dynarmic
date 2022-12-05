/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

namespace oaknut {
struct PointerCodeGeneratorPolicy;
template<typename>
class BasicCodeGenerator;
using CodeGenerator = BasicCodeGenerator<PointerCodeGeneratorPolicy>;
struct Label;
}  // namespace oaknut

namespace Dynarmic::IR {
enum class AccType;
class Inst;
}  // namespace Dynarmic::IR

namespace Dynarmic::Backend::Arm64 {

struct EmitContext;
enum class LinkTarget;

bool IsOrdered(IR::AccType acctype);
void EmitReadMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn);
void EmitReadMemory128(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn);
void EmitExclusiveReadMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn);
void EmitExclusiveReadMemory128(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn);
void EmitWriteMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn);
void EmitExclusiveWriteMemory(oaknut::CodeGenerator& code, EmitContext& ctx, IR::Inst* inst, LinkTarget fn);

}  // namespace Dynarmic::Backend::Arm64
