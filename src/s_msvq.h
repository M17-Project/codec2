#ifndef S_MSVQ_CODEC2
#define S_MSVQ_CODEC2

/*------------------------------------------------*\
Field						N			bits
Coarse codebook	index		64			6
3-stage MSVQ for LSF1..3	3*4			12
4-stage MSVQ for LSF4..6	4*4			16
3-stage MSVQ for LSF7..10	3*4			12
Voicing						2			2
Energy						256			8
Pitch						256			8

Total									64

Note:
Original Codec 2 allocates 5 bits for the energy
field and 7 bits for pitch. Voicing stays the same.
Codec 2 uses 50 bits for LSFs alone. My proposal
is to use 46 bits, 6 for the initial, coarse
approximation followed by split 3/4-stage refinement.
Codebook sizes for the split MSVQ are always 2^4=16.

Wojciech Kaczmarski, SP5WWP
M17 Project
21/01/2022
\*------------------------------------------------*/

//codebook sizes in bits
#define Q0_SIZE		6
#define Q1_SIZE		4
#define Q2_SIZE		4
#define Q3_SIZE		4

//stages
#define Q1_STAGES	3
#define Q2_STAGES	4
#define Q3_STAGES	3

//-----------------------q0------------------------
//Q codebook for the initial (coarse) quantization
extern const float cb_Q[64][10];

//-----------------------MSVQ-----------------------
//------------------------q1------------------------
//Qf1_1 codebook
//1st stage for LSF1..3
extern const float Qf1_1[16][3];

//Qf1_2 codebook
//2nd stage for LSF1..3
extern const float Qf1_2[16][3];

//Qf1_3 codebook
//3rd stage for LSF1..3
extern const float Qf1_3[16][3];

//------------------------q2------------------------
//Qf2_1 codebook
//1st stage for LSF4..6
extern const float Qf2_1[16][3];

//Qf2_2 codebook
//2nd stage for LSF4..6
extern const float Qf2_2[16][3];

//Qf2_3 codebook
//3rd stage for LSF4..6
extern const float Qf2_3[16][3];

//Qf2_4 codebook
//4th stage for LSF4..6
extern const float Qf2_4[16][3];

//------------------------q3------------------------
//Qf3_1 codebook
//1st stage for LSF7..10
extern const float Qf3_1[16][4];

//Qf3_2 codebook
//2nd stage for LSF7..10
extern const float Qf3_2[16][4];

//Qf3_3 codebook
//3rd stage for LSF7..10
extern const float Qf3_3[16][4];

#endif
