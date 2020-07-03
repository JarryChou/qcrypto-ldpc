LDPC Protocol write-up {#ldpc_readme}
====

This README is located in `qcrypto/errorcorrection/`.

The purpose of this readme is to pen down thoughts, notes & implementation on the LDPC error correction algorithm.

## Theory

Definitions:
| Symbol  | Size  | Name  | Equivalent to  |
|---|---|---|---|
k	| k		    |   Number of bits of info                    |  
n	| n		    |   Number of bits of encoded data            |  
?	| (n, k)	|	Linear code of length n and dimension k   |  
u  	| (1* x k) 	|   Information vector (data)                 |  
G	| (k x n)	|	Generator matrix			              |  Any matrix whose rows form basis of codeword space
c  	| (1* x n) 	|   Codeword (sent data)			          |  c = uG
C	| -		    |   Codeword space                            |  
H	| (m x n)	|	Parity check matrix			              |  C is the kernel of 

1*: Presumed to be 1

Relationships:
* Rule: For any codeword c, Hc^T = 0
* Derived rule: Since G is the matrix whose rows form basis of codeword space C, HG^T = 0

Notes: 
1. G and H are binary matrices; that is, in theory, they are matrices with 0s and 1s.
2. A binary low-density parity-check (LDPC) code is a binary linear block code with a parity-check matrix that is a sparse matrix.
	i.e. H has a low density of 1s.

Original high-level conceptual procedure (See both LDPC dessert [2] & LDPC for dummies [1])
	Concept: Use LDPC itself to correct the entire message
1. Let Alice be the sender, Bob be the receiver.
2. Both Alice & Bob agree upon two matrices, G and H prior to the conversation, s.t. HG^T = 0.
3. Alice has a sequence of data she wants to send to Bob. (Let's call this d)
4. Alice calculates an additional sequence of parity bits (let's call this p)
5. Alice appends p to d. Note that d + p = u.
6. To obtain the encoded c to send to Bob, Alice does matrix multiplication: c = uG
7. Alice sends c to Bob through the noisy channel, who receives c in some form of corrupted format.
8. Bob knows that Hc^T = 0, so he somehow does correction (through belief propagation / iterative message-passing decoding) to correct the packet until Hc^T = 0

QKD high-level conceptual procedure (See: LDPC for dummies [1])
* Concept: Use LDPC as a checksum i.e. Alice sends c over to Bob who verifies if his c is equal to Alice's
1. Both Alice & Bob agree upon the matrices G and H prior to the conversation, s.t. HG^T = 0. These can be kept static.
2. Both Alice & Bob have a raw key after sifting.
3. Alice takes a block of her key as the information vector u1 and computes a codeword c1 = u1G.
	However in this case, c1 is used as a checksum.
4. Alice sends c1, as well as a hash of her key to Bob
5. Bob computes his own codeword c2 using a block of his own key and compares c1 and c2. 
6. Bob iteratively corrects his key s.t. c1 == c2 using the same procedure as above until he gets it or he reaches maximum # of iterations
7. Using the hash of the key sent by Alice, Bob verifies his key is likely equal to Alice's

Notes:
1. LDPC requires that a sufficiently large amount of sifted raw data is acquired beforehand or the hash can be potentially bruteforced
2. What should the matrices be s.t. they are useful as checksums? 
	a. How are we assured that Eve cannot obtain any information from c1?
		Sifted keys used must be longer than matrix
	b. How to obtain good error correction performance?
		1. Use matrices used in standards (but these matrices are no longer just 0s and 1s) that I don't really understand yet
		2. Use random sparse matrices (easiest solution can be improved though I think)
		3. Use existing sparse matrices we have access to
	c. How does the belief propagation work?
		We already have this in pseudo-code lmao but I don't really recall how it works

## Procedure

## Variables needed

* Initiator (Alice)
* Follower (Bob)

## References
1. LDPC for dummies	https://arxiv.org/ftp/arxiv/papers/1205/1205.4977.pdf
2. LDPC dissert.		https://escholarship.org/uc/item/3862381k