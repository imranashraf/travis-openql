.. _classical_instructions:

Classical Instructions
======================

OpenQL supports a mix of quantum and classical computing at the gate level.
Please recall that classical gates are gates that don't have any qubit as operand,
only zero or more classical registers and execute in classical hardware.

Let us first look at some example code (taken from *tests/test_hybrid.py*):

.. code:: python

   num_qubits = 5
   num_cregs = 10

   p = ql.Program('test_classical', platform, num_qubits, num_cregs)

   k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)

   # create classical registers
   rd = ql.CReg()
   rs1 = ql.CReg()
   rs2 = ql.CReg()

   # add/sub/and/or/xor
   k1.classical(rd, ql.Operation(rs1, '+', rs2))

   # not
   k1.classical(rd, ql.Operation('~', rs2))

   # comparison
   k1.classical(rd, ql.Operation(rs1, '==', rs2))

   # initialize (rd = 2)
   k1.classical(rd, ql.Operation(2))

   # assign (rd = rs1)
   k1.classical(rd, ql.Operation(rs1))

   # measure
   k1.gate('measure', [0], rs1)

   # add kernel
   p.add_kernel(k1)
   p.compile()

In this, we see a few new methods:

- ql.CReg():
  Get a free classical register (*creg*) using the classical register constructor.
  The corresponding destructor would free it again.

- k.classical(creg, operation):
  Create a classical gate, assigning the value of the *operation* to the specified destination classical register.
  The destination classical register and any classical registers that are operands to the operation must have indices that are less than the number of classical registers specified with the creation of kernel *k*.
  The gate is added to kernel *k*'s circuit.

- ql.Operation(value):
  Create an operation loading the immediate value *value*.

- ql.Operation(creg):
  Create an operation loading the value of classical register *creg*.

- ql.Operation(operator, creg):
  Create an operation applying the unary operator *operator* on the value of classical register *creg*.

- ql.Operation(creg1, operator, creg2):
  Create an operation applying the binary operator *operator* on the values of classical registers *creg1* and *creg2*.

The operators in the calls above are a string with the name of one of the familiar C operators: the binary operators
*+*, *-*, *&*, *|*, *^*, *==*, *!=*, *<*, *>*, *<=*, and *>=*; or the unary *~*.

Please note the creation of the quantum measurement gate that takes a classical register as operand to store the result.

.. _classical_gate_attributes_in_the_internal_representation:

Classical gate attributes in the internal representation
---------------------------------------------------------

A classical gate has all general gate attributes, of which some are not used, and one additional one:

+---------------+-----------+--------------------+------------+------------+----------------+
| Attribute     | kind      | example            | used by    | updated by | C++ type       |
+===============+===========+====================+============+============+================+
| name          | structural| "add"              | all passes | never      | string         |
+---------------+           +--------------------+            +            +----------------+
| creg_operands |           | [r0,r1]            |            |            | vector<size_t> |
+---------------+           +--------------------+            +            +----------------+
| int_operand   |           | 3                  |            |            | int            |
+---------------+           +--------------------+            +            +----------------+
| type          |           | __classical_gate__ |            |            | gate_type_t    |
+---------------+-----------+--------------------+------------+            +----------------+
| duration      | semantic  | 20                 | schedulers,|            | size_t         |
|               |           |                    | etc.       |            |                |
+---------------+-----------+--------------------+------------+            +----------------+
| cycle         | result    | 4                  | code       | scheduler  | size_t         |
|               |           |                    | generation |            |                |
+---------------+-----------+--------------------+------------+------------+----------------+
| operands      |           |                    | never      |            |                |
+---------------+           +                    +            +            +                +
| angle         |           |                    |            |            |                |
+---------------+           +                    +            +            +                +
| mat           |           |                    |            |            |                |
+---------------+-----------+--------------------+------------+------------+----------------+

Some further notes on the gate attributes:

- ``name``: The internal name. Happens to correspond to the gate name in the output QASM representation.

- ``creg_operands``: Please note that for all gates the classical operands are in the *creg_operands* attribute, and the quantum operands are in the *operands* attribute.

- ``int_operand``: An immediate integer valued operand is kept here.

- ``type``: Is always *__classical_gate__*. Classical gates are distinguished by their name.

:Note: That classical gates are distinguished by their name and not by some type, is not as problematic as for quantum gates. The names of classical gates are internal to OpenQL and have no relation to an external representation.

- ``duration``: Has a built-in value of 20.

:Note: That the value of duration is built-in, is strange. A first better value would be *cycle_time*.

- ``operands``, ``angle``, and ``mat`` are not used as attributes by classical gates.


The following classical gates are supported:

+-------+---------------------------------+----------------+---------------+--------------------------------------------+
| name  | operands                        | operation type | inv operation | OpenQL example                             |
+=======+=================================+================+===============+============================================+
| "add" | 1 dest and 2 src reg indices    | ARITHMETIC     |               | k.classical(rd, Operation(rs1, '+', rs2))  |
+-------+                                 +                +               +--------------------------------------------+
| "sub" |                                 |                |               | k.classical(rd, Operation(rs1, '-', rs2))  |
+-------+                                 +----------------+---------------+--------------------------------------------+
| "eq"  |                                 | RELATIONAL     | "ne"          | k.classical(rd, Operation(rs1, '==', rs2)) |
+-------+                                 +                +---------------+--------------------------------------------+
| "ne"  |                                 |                | "eq"          | k.classical(rd, Operation(rs1, '!=', rs2)) |
+-------+                                 +                +---------------+--------------------------------------------+
| "lt"  |                                 |                | "ge"          | k.classical(rd, Operation(rs1, '<', rs2))  |
+-------+                                 +                +---------------+--------------------------------------------+
| "gt"  |                                 |                | "le"          | k.classical(rd, Operation(rs1, '>', rs2))  |
+-------+                                 +                +---------------+--------------------------------------------+
| "le"  |                                 |                | "gt"          | k.classical(rd, Operation(rs1, '<=', rs2)) |
+-------+                                 +                +---------------+--------------------------------------------+
| "ge"  |                                 |                | "lt"          | k.classical(rd, Operation(rs1, '>=', rs2)) |
+-------+                                 +----------------+---------------+--------------------------------------------+
| "and" |                                 | BITWISE        |               | k.classical(rd, Operation(rs1, '&', rs2))  |
+-------+                                 +                +               +--------------------------------------------+
| "or"  |                                 |                |               | k.classical(rd, Operation(rs1, '|', rs2))  |
+-------+                                 +                +               +--------------------------------------------+
| "xor" |                                 |                |               | k.classical(rd, Operation(rs1, '^', rs2))  |
+-------+---------------------------------+                +               +--------------------------------------------+
| "not" | 1 dest and 1 src reg index      |                |               | k.classical(rd, Operation('~', rs))        |
+-------+                                 +----------------+               +--------------------------------------------+
| "mov" |                                 | ARITHMETIC     |               | k.classical(rd, Operation(rs))             |
+-------+---------------------------------+                +               +--------------------------------------------+
| "ldi" | 1 dest reg index, 1 int_operand |                |               | k.classical(rd, Operation(3))              |
+-------+---------------------------------+----------------+               +--------------------------------------------+
| "nop" | none                            | undefined      |               | k.classical('nop')                         |
+-------+---------------------------------+----------------+---------------+--------------------------------------------+

In the above:

``Operation()`` creates an expression (binary, unary, register, or immediate); apart from in the OpenQL interface as shown above, it is also used as expression in the internal representation of the *br_condition* attribute of a kernel

``operation type`` indicates the type of operation which is mainly used for checking

``inv operation`` represents the inverse of the operation; it is used in code generation of conditional branching; see :ref:`kernel`


Classical gates in circuits and bundles in the internal representation
----------------------------------------------------------------------

In circuits and bundles, no difference is made between classical and quantum gates.
Classical gates are scheduled based on their operands and duration.
The ``cycle`` attribute reflects the cycle in which the gate is executed, as usual.

Scheduling of classical instructions is assigning cycle values to these so that the register dependences of these are guaranteed to be met (ordinary scheduler); when resource constraints would be involved, those should be adhered to as well (rcscheduler). The ``cycle_time`` would have to be the greatest common divider of the ``duration`` of all gates, classical and quantum.

Classical instructions may depend on quantum gates when they retrieve the result of measurement.
Quantum gates may have a control dependence on classical code because of a conditional branch; with immediate feedback, in which a single gate is performed conditionally on the value of a classical register, there also is a dependence of a quantum gate on a classically computed value.

From these dependences, an exact cycle value of the start of execution of each gate can be computed,
relative to the start of execution of a kernel/circuit.
Any constraints (maximum number of classical instructions to start in one cycle, maximum number of quantum gates to start in one cycle,
overlapping resource uses) have to encoded in resources which are then adhered to by the rcscheduler.

.. _classical_input_external_representation:

Input external representation
-----------------------------

OpenQL supports as input external representation currently only the OpenQL program, written in C++ and/or Python.
See :ref:`input_external_representation`.

Classical gates are created using an API of the form as shown above in :ref:`classical_instructions`.
The table above shows the correspondence between the input external and internal representation.

:Note: There is no role for the configuration file in creating classical gates. This is a lost opportunity because it would have harmonized classical and quantum gates more. When defining QASM as input external representation, this might be revised.


.. _classical_output_external_representation:

Output external representation
------------------------------

There are two closely related output external representations supported, both dialects of QASM 1.0;
see :ref:`output_external_representation`: sequential and bundled QASM.
Again, these don't make a difference between classical and quantum gates.

The following table shows the QASM representation of a single classical gate:

+-------+-----------------------------------------------------+---------------------+
| name  | example operands                                    | QASM representation |
+=======+=====================================================+=====================+
| "add" | 0 as dest reg index, 1 and 2 as source reg indices  | add r0, r1, r2      |
+-------+                                                     +---------------------+
| "sub" |                                                     | sub r0, r1, r2      |
+-------+                                                     +---------------------+
| "and" |                                                     | and r0, r1, r2      |
+-------+                                                     +---------------------+
| "or"  |                                                     | or r0, r1, r2       |
+-------+                                                     +---------------------+
| "xor" |                                                     | xor r0, r1, r2      |
+-------+                                                     +---------------------+
| "eq"  |                                                     | eq r0, r1, r2       |
+-------+                                                     +---------------------+
| "ne"  |                                                     | ne r0, r1, r2       |
+-------+                                                     +---------------------+
| "lt"  |                                                     | lt r0, r1, r2       |
+-------+                                                     +---------------------+
| "gt"  |                                                     | gt r0, r1, r2       |
+-------+                                                     +---------------------+
| "le"  |                                                     | le r0, r1, r2       |
+-------+                                                     +---------------------+
| "ge"  |                                                     | ge r0, r1, r2       |
+-------+-----------------------------------------------------+---------------------+
| "not" | 0 as dest reg index, 1 as source reg index          | not r0, r1          |
+-------+                                                     +---------------------+
| "mov" |                                                     | mov r0, r1          |
+-------+-----------------------------------------------------+---------------------+
| "ldi" | 0 as dest reg index, 3 as int_operand               | ldi r0, 3           |
+-------+-----------------------------------------------------+---------------------+
| "nop" | none                                                | nop                 |
+-------+-----------------------------------------------------+---------------------+


