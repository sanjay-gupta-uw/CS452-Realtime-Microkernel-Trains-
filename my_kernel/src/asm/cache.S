// LSR = Logical Shift Right -> 1000 -> 0100 (divide by 2^{n})
.global InvalidateANDCleanDCache
InvalidateANDCleanDCache:
    DSB SY                 // Ensure memory operations complete
    MRS X0, CLIDR_EL1       // Read Cache Level ID Register
    ANDS X1, X0, #0x7000000 // Extract LoC (Level of Coherence)
    LSR X1, X1, #23
    CBZ X1, done            // If LoC == 0, exit

    MOV X2, #0             // Start at level 0
level_loop:

    MOV X3, X2             // Select cache level
    LSL X3, X3, #1
    MSR CSSELR_EL1, X3     // Select cache level in CSSELR_EL1
    ISB                    // Ensure register change takes effect

    MRS X4, CCSIDR_EL1     // Read Cache Size ID Register

    // Extract cache line size (LineSize field)
    AND X5, X4, #0x7       // Extract bits [2:0]
    ADD X5, X5, #4         // Convert to log2(byte size)
    MOV X6, #1
    LSL X5, X6, X5         // Compute actual cache line size (1 << LineSize)

    // Extract number of sets (NumSets field)
    LSR X7, X4, #13        // Extract bits [27:13]
    AND X7, X7, #0x7FFF    // Mask to get NumSets (0x7FFF)

    // Extract associativity (Associativity field)
    LSR X8, X4, #3         // Extract bits [12:3]
    AND X8, X8, #0x3FF     // Mask to get Associativity
    ADD X8, X8, #1         // Associativity = field + 1

    MOV X9, #0             // Way index
way_loop:
    MOV X10, #0            // Set index
set_loop:
    LSL X11, X9, #30       // Encode way index
    ORR X11, X11, X10, LSL #5 // Encode set index
    ORR X11, X11, X2, LSL #1 // Encode cache level
    DC CISW, X11           // Clean and Invalidate Set/Way

    ADD X10, X10, #1       // Increment set index
    CMP X10, X7
    B.LT set_loop          // Repeat for all sets

    ADD X9, X9, #1         // Increment way index
    CMP X9, X8
    B.LT way_loop          // Repeat for all ways

    ADD X2, X2, #1         // Move to next cache level
    CMP X2, X1
    B.LT level_loop        // Repeat for all cache levels

done:
    DSB SY                 // Ensure completion
    ISB

    RET
