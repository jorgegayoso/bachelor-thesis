/**
 * @name Pattern memset broad: memset spans struct field
 * @kind problem
 * @problem.severity warning
 * @id thesis/pattern-memset-broad
 * @tags ioo bulk-write memset
 */

import cpp

class MemsetCall extends FunctionCall {
  MemsetCall() {
    this.getTarget().getName() = ["memset", "__builtin_memset"]
  }
}

/**
 * The destination expression names a struct field, either as:
 *   - `&x.field` / `&x->field` (address-of), or
 *   - `x.arr` / `x->arr` (array field decays to pointer).
 */
predicate destIsField(Expr dest, FieldAccess fa) {
  fa = dest.(AddressOfExpr).getOperand()
  or
  fa = dest and fa.getType() instanceof ArrayType
}

from
  MemsetCall call,
  FieldAccess fa,
  Field field,
  int fieldSize,
  int writeSize
where
  destIsField(call.getArgument(0), fa) and
  field = fa.getTarget() and
  fieldSize = field.getType().getSize() and
  writeSize = call.getArgument(2).getValue().toInt() and
  writeSize > fieldSize
select call,
  "Pattern memset broad: memset writes " + writeSize +
    " bytes into field $@ (" + fieldSize +
    " bytes) of struct $@ in $@",
  field, field.getName(),
  field.getDeclaringType(), field.getDeclaringType().getName(),
  call.getEnclosingFunction(), call.getEnclosingFunction().getName()
