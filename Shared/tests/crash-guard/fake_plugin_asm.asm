.code
; Has unwind data with rbp as its frame pointer, then points rbp at unmapped memory and faults:
; unwinding this frame reads through rbp, which is what a stack overwrite does to a real plugin.
asm_bad_frame PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .setframe rbp, 0
    .endprolog
    mov rbp, 10h
    xor eax, eax
    mov eax, dword ptr [rax]
    mov rsp, rbp
    pop rbp
    ret
asm_bad_frame ENDP
END
