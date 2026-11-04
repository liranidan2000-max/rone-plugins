.code
; Code with no unwind data, like a JIT's: pushes its argument, a pointer into the plugin's data, then
; faults. Walking past it reads raw stack, where that pointer looks like a return address.
asm_foreign_crash PROC
    push rcx
    xor eax, eax
    mov eax, dword ptr [rax]
    pop rcx
    ret
asm_foreign_crash ENDP
END
