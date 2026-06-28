/**
 * @name Pattern memset fine-grained: memset spans struct field (sizeof-based)
 * @kind problem
 * @problem.severity warning
 * @id thesis/pattern-memset
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

/** `t` is `field`'s declaring type, or transitively contains it. */
predicate encloses(Type t, Field field) {
  t = field.getDeclaringType()
  or
  exists(Field outerField |
    outerField.getDeclaringType() = t and
    encloses(outerField.getType().stripType(), field)
  )
}

/**
 * True when `sizeArg` contains a sizeof referencing the enclosing
 * struct of `field` anywhere in its AST subtree.
 */
predicate sizeReferencesEnclosingStruct(Expr sizeArg, Field field) {
  exists(Type t |
    encloses(t, field) and
    (
      sizeArg.getAChild*().(SizeofExprOperator).getExprOperand().getType().stripType() = t
      or
      sizeArg.getAChild*().(SizeofTypeOperator).getTypeOperand().stripType() = t
    )
  )
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
  writeSize > fieldSize and
  sizeReferencesEnclosingStruct(call.getArgument(2), field)
select call,
  "Pattern memset: memset writes " + writeSize +
    " bytes into field $@ (" + fieldSize +
    " bytes) of struct $@ in $@",
  field, field.getName(),
  field.getDeclaringType(), field.getDeclaringType().getName(),
  call.getEnclosingFunction(), call.getEnclosingFunction().getName()
