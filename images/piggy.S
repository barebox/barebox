        .section .piggydata,#alloc
        .globl  input_data
input_data:
        .incbin "images/barebox.z"
        .globl  input_data_end
input_data_end:
