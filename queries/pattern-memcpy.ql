/**
 * @name Pattern memcpy fine-grained: memcpy/memmove spans struct field (both sides are fields)
 * @kind problem
 * @problem.severity warning
 * @id thesis/pattern-memcpy
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
  FieldAccess destFa,
  FieldAccess srcFa,
  Field destField,
  Field srcField,
  int destFieldSize,
  int srcFieldSize,
  int writeSize
where
  writeSize = call.getArgument(2).getValue().toInt() and
  argIsField(call.getArgument(0), destFa) and
  argIsField(call.getArgument(1), srcFa) and
  destField = destFa.getTarget() and
  srcField = srcFa.getTarget() and
  destFieldSize = destField.getType().getSize() and
  srcFieldSize = srcField.getType().getSize() and
  (writeSize > destFieldSize or writeSize > srcFieldSize)
select call,
  "Pattern memcpy: " + call.getTarget().getName() + " copies " + writeSize +
    " bytes — dest field $@ (" + destFieldSize +
    " bytes) of struct $@, source field $@ (" + srcFieldSize +
    " bytes) of struct $@ in $@",
  destField, destField.getName(),
  destField.getDeclaringType(), destField.getDeclaringType().getName(),
  srcField, srcField.getName(),
  srcField.getDeclaringType(), srcField.getDeclaringType().getName(),
  call.getEnclosingFunction(), call.getEnclosingFunction().getName()
