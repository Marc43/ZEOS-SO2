Test[0]: [testGetPID] TEST GETPID
Test[1]: [testMemP0] TEST PHYSICAL MEMORY INITIALIZATION P1
Test[2]: [testNiceRes] CHECK NICE RESULT
Test[3]: [testGetStatsCurrent] GET_STATS(pid is current process)
Test[4]: [testAccountCS] GET_STATS accounts context switches
Test[5]: [testAccountCSwoBlock] GET_STATS accounts context switches without blocking
Test[6]: [testTicsAccountingRemaining] GET_STATS DECREASING REMAINING TICKS FOR RUNNING PROCESSES
Test[7]: [testFatherResultFork] FORK FATHER RESULT
Test[8]: [testChildResultFork] FORK CHILD RESULT
Test[9]: [testForkInheritQuantum] FORK INHERIT QUANTUM
Test[10]: [testForkResetStats] FORK RESET STATS
Test[11]: [testTicsAccountingRunning] GET_STATS INCREASING TICKS FOR RUNNING PROCESSES
Test[12]: [testNiceFunctionality] NICE EFFECTS ON SCHEDULING
Test[13]: [testCopyGlobalMemory1] FORK DATA COPY: INHERIT AND DO NOT SHARE WITH FATHER)
Test[14]: [testCopyGlobalMemory2] FORK DATA COPY: INHERIT AND DO NOT SHARE WITH BROTHERS
Test[15]: [testForkHierarchy] FORK PROCESS HIERARCHY CREATION
Test[16]: [testExitFreesMem] TEST EXIT FREES DATA FRAMES
Test[17]: [testExit] EXIT REGULAR PROCES
Test[18]: [testOutOfRangeFork] FORK OUT OF RANGE DETECTION
Test[19]: [testForkReuse] FORK DATA STRUCTURES RECYCLING AFTER EXIT
Test[20]: [testRoundRobinQuantumRem] FULL QUANTUM AFTER CTX SWITCH
Test[21]: [testRoundRobinSwithcNb] ROUND ROBIN
Test[22]: [testTicsAccountingBlocked] GET_STATS INCREASING TICKS FOR BLOCKED PROCESSES
Test[23]: [testGetStatsInvalidPID] GET_STATS(pid parameter is negative)
Test[24]: [testGetStatsUnknownPID] GET_STATS(pid parameter does not exist)
Test[25]: [testGetStatsReady] GET_STATS(process is ready)
Test[26]: [testGetStatsBlocked] GET_STATS(process is blocked)
Test[27]: [testGetStatsDead] GET_STATS(process is dead)
Test[28]: [testGetStatsInvalidAddress] GET_STATS(address parameter is NULL)
Test[29]: [testGetStatsInvalidAddress1] GET_STATS(address parameter in kernel address space)
Test[30]: [testGetStatsInvalidAddress2] GET_STATS(address parameter outside user address space)
Test[31]: [testIndependentQuantums] FORK FATHER AND CHILDREN DO NOT SHARE QUANTUM

NICE: Check the additional system call handler is installed.
[testNiceRes] CHECK NICE RESULT: -

GETPID
[testGetPID] test getpid: -

FORK
[testMemP0] TEST PHYSICAL MEMORY INITIALIZATION P0: exit
[testFatherResultFork] fork father result: getpid/exit
[testChildResultFork] fork child result: getpid/exit
[testForkInheritQuantum] fork inherit quantum: getpid/exit  
[testIndependentQuantums] fork father and children do not share quantum: getpid/exit
[testForkResetStats] fork reset stats: getstats/gepid/exit
[testOutOfRangeFork] fork out of range detection:exit
[testForkReuse] fork data structures recycling after exit: exit
[testCopyGlobalMemory1] fork data copy: inherit and do not share with father: getpid/exit
[testCopyGlobalMemory2] fork data copy: inherit and do not share with brothers: getpid/exit
[testForkHierarchy] fork process hierarchy creation: getpid/exit

EXIT
[testExit] exit(regular process): fork
[testExitFreesMem] exit(free data frames): fork

ROUND ROBIN
[testRoundRobinSwithcNb] ROUND ROBIN:  fork/get_stats
[testRoundRobinQuantumRem] FULL QUANTUM AFTER CTX SWITCH: fork/exit/getpid/get_stats

GET_STATS
[testGetStatsInvalidPID] GET_STATS(pid parameter is negative): -
[testGetStatsUnknownPID] GET_STATS(pid parameter does not exist): -
[testGetStatsInvalidAddress] GET_STATS(address parameter is NULL): -
[testGetStatsInvalidAddress1] GET_STATS(address parameter in kernel address space): -
[testGetStatsInvalidAddress2] GET_STATS(address parameter outside user address space): -
[testGetStatsCurrent] GET_STATS(pid is current process): getpid
[testGetStatsReady] GET_STATS(process is ready): getpid/fork
[testGetStatsDead] GET_STATS(process is dead): fork/exit
[testGetStatsBlocked] GET_STATS(process is blocked): fork/exit
[testTicsAccountingBlocked] GET_STATS INCREASING TICKS FOR BLOCKED PROCESSES: fork/exit
[testTicsAccountingRunning] GET_STATS INCREASING TICKS FOR RUNNING PROCESSES: getpid
[testAccountCS] GET_STATS accounts context switches: fork/zeos_sem_init/zeos_sem_signal/zeos_sem_wait/exit
[testAccountCSwoBlock] GET_STATS accounts context switches without blocking: getpid
[testTicsAccountingRemaining] GET_STATS DECREASING REMAINING TICKS FOR RUNNING PROCESSES: getpid
[testNiceFunctionality] NICE EFFECTS ON SCHEDULING: fork/exit/nice/zeos_sem_init/zeos_sem_wait/zeos_sem_signal

