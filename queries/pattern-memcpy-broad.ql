/**
 * @name Pattern memcpy broad: memcpy/memmove spans struct field
 * @kind problem
 * @problem.severity warning
 * @id thesis/pattern-memcpy-broad
 * @tags ioo bulk-write memcpy
 */

import cpp

class MemcpyCall extends FunctionCall {
  MemcpyCall() {
    this.getTarget().getName() =
      ["memcpy", "memmove", "__builtin_memcpy", "__builtin_memmove"]
  }
}

/**
 * The expression names a struct field, either as:
 *   - `&x.field` / `&x->field` (address-of), or
 *   - `x.arr` / `x->arr` (array field decays to pointer).
 */
predicate argIsField(Expr arg, FieldAccess fa) {
  fa = arg.(AddressOfExpr).getOperand()
  or
  fa = arg and fa.getType() instanceof ArrayType
}

from
  MemcpyCall call,
  FieldAccess fa,
  Field field,
  int fieldSize,
  int writeSize,
  string side
where
  writeSize = call.getArgument(2).getValue().toInt() and
  (
    // Destination-side overflow
    argIsField(call.getArgument(0), fa) and
    side = "dest"
    or
    // Source-side overflow
    argIsField(call.getArgument(1), fa) and
    side = "source"
  ) and
  field = fa.getTarget() and
  fieldSize = field.getType().getSize() and
  writeSize > fieldSize
select call,
  "Pattern mempcy broad: " + call.getTarget().getName() + " copies " + writeSize +
    " bytes from/to " + side + " field $@ (" + fieldSize +
    " bytes) of struct $@ in $@",
  field, field.getName(),
  field.getDeclaringType(), field.getDeclaringType().getName(),
  call.getEnclosingFunction(), call.getEnclosingFunction().getName()
