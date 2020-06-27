Error Correction {#ec_readme}
====

This README is located in `qcrypto/errorcorrection/`.

# DESCRIPT
You may notice that the code style in ecd2 appears a little different from the other components; specifically, it uses CamelCase instead of snake_case (with the
exception of fully capitalized constants) as there were many variables which did not abide to either case. However, the filenames retain their snake case qualities to fit with the other components.



THE MAIN ECD2 PROGRAM
* ecd2.h

COMPONENTS OF ECD2
* cascade_biconf.h
* comms.h
* debug.h
* helpers.h
* priv_amp.h
* qber_estim.h
* rnd.h
* processblock_mgmt.h

DEFINITION FILES
* packets.h
* processblock.h
* proc_state.h
* globalvars.h

Internals Procedure
@startuml
    Test->Test2  : Command()
    Test<--Test2 : Ack()
@enduml
