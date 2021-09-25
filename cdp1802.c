#include "cdp1802.h"

#include <stdint.h>
#include <stdio.h>

/*
 * Coded from a datasheet found here: http://www.qq22.net/qq22/pdf/1802.pdf
 */

uint8_t M[0x10000];
uint16_t R[0x10];
uint8_t D;
int DF;
uint8_t P;
uint8_t X;
uint8_t T;
int IE;
int Q;
int EF1;
int EF2;
int EF3;
int EF4;

#define AS(X, Y) { Y = X; }
#define ATLO(X, Y)
#define ATHI(X, Y)

int step(void)
{
   uint8_t opcode = M[R[P]];

   ++R[P];

   uint8_t I = opcode >> 4 * 1 & 0xf;
   uint8_t N = opcode >> 4 * 0 & 0xf;

   if (0)
   {
      printf("I:0x%x N:0x%x\n", I, N);
   }

   if (0x00 == opcode)
   {
      // 00    WAIT FOR DMA OR INTERRUPT; M[R[0]] -> BUS                                                              //  00 IDLE                                    IDL
      return 0;
   }
   else if (0x00 == I)
   {
      // 0N    M[R[N]] -> D;                                                                                          //  0N LOAD VIA N                              LDN
      AS(M[R[N]], D);
   }
   else if (0x01 == I)
   {
      // 1N    R[N] + 1 -> R[N]                                                                                       //  1N INCREMENT REG N                         INC
      AS(R[N] + 1, R[N]);
   }
   else if (0x02 == I)
   {
      // 2N    R[N] - 1 -> R[N]                                                                                       //  2N DECREMENT REG N                         DEC
      AS(R[N] - 1, R[N]);
   }
   else if (0x04 == I)
   {
      // 4N    M[R[N]] -> D; R[N] + 1 -> R[N]                                                                         //  4N LOAD ADVANCE                            LDA
      AS(M[R[N]], D);
      AS(R[N] + 1, R[N]);
   }
   else if (0x05 == I)
   {
      // 5N    D -> M[R[N]]                                                                                           //  5N STORE VIA N                             STR
      AS(D, M[R[N]]);
   }
   else if (0x08 == I)
   {
      // 8N    R[N].0 -> D                                                                                            //  8N GET LOW REG N                           GLO
      D = R[N] >> 0 * 8;
   }
   else if (0x09 == I)
   {
      // 9N    R[N].1 -> D                                                                                            //  9N GET HIGH REG N                          GHI
      D = R[N] >> 1 * 8;
   }
   else if (0x0a == I)
   {
      // AN    D -> R[N].0                                                                                            //  AN PUT LOW REG N                           PLO
      int byte = 0;
      R[N] &= 0xff << (1 - byte) * 8;
      R[N] |= (uint16_t)D << byte * 8;
   }
   else if (0x0b == I)
   {
      // BN    D -> R[N].1                                                                                            //  BN PUT HIGH REG N                          PHI
      int byte = 1;
      R[N] &= 0xff << (1 - byte) * 8;
      R[N] |= (uint16_t)D << byte * 8;
   }
   else if (0x0d == I)
   {
      // DN    N -> P                                                                                                 //  DN SET P                                   SEP
      AS(N, P);
   }
   else if (0x0e == I)
   {
      // EN    N -> X                                                                                                 //  EN SET X                                   SEX
      AS(N, X);
   }
   else if (0x38 == opcode)
   {
      // 38    R[P] + 1 -> R[P]                                                                                       //  38 SHORT SKIP                              SKP
      AS(R[P] + 1, R[P]);
   }
   else if (0x03 == I)
   {
      int test = 0;

      // 30                 M[R[P]] -> R[P].0                                                                         //  30 SHORT BRANCH                            BR
      // 31    IF Q == 1,   M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  31 SHORT BRANCH IF Q == 1                  BQ
      // 32    IF D == 0,   M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  32 SHORT BRANCH IF D == 0                  BZ
      // 33    IF DF == 1,  M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  33 SHORT BRANCH IF DF == 1                 BDF
      // 34    IF EF1 == 1, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  34 SHORT BRANCH IF EF1 == 1                B1
      // 35    IF EF2 == 1, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  35 SHORT BRANCH IF EF2 == 1                B2
      // 36    IF EF3 == 1, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  36 SHORT BRANCH IF EF3 == 1                B3
      // 37    IF EF4 == 1, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  37 SHORT BRANCH IF EF4 == 1                B4
      // 39    IF Q == 0,   M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  39 SHORT BRANCH IF Q == 0                  BNQ
      // 3A    IF D != 0,   M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  3A SHORT BRANCH IF D != 0                  BNZ
      // 3B    IF DF == 0,  M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  3B SHORT BRANCH IF DF == 0                 BNF
      // 3C    IF EF1 == 0, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  3C SHORT BRANCH IF EF1 == 0                BN1
      // 3D    IF EF2 == 0, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  3D SHORT BRANCH IF EF2 == 0                BN2
      // 3E    IF EF3 == 0, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  3E SHORT BRANCH IF EF3 == 0                BN3
      // 3F    IF EF4 == 0, M[R[P]] -> R[P].0, ELSE R[P] + 1 -> R[P]                                                  //  3F SHORT BRANCH IF EF4 == 0                BN4
      if (opcode == 0x30)
      {
         test = 1;
      }
      if (opcode == 0x31)
      {
         test = Q == 1;
      }
      if (opcode == 0x32)
      {
         test = D == 0;
      }
      if (opcode == 0x33)
      {
         test = DF == 1;
      }
      if (opcode == 0x34)
      {
         test = EF1 == 1;
      }
      if (opcode == 0x35)
      {
         test = EF2 == 1;
      }
      if (opcode == 0x36)
      {
         test = EF3 == 1;
      }
      if (opcode == 0x37)
      {
         test = EF4 == 1;
      }

      if (opcode == 0x39)
      {
         test = Q == 0;
      }
      if (opcode == 0x3a)
      {
         test = D != 0;
      }
      if (opcode == 0x3b)
      {
         test = DF == 0;
      }
      if (opcode == 0x3c)
      {
         test = EF1 == 0;
      }
      if (opcode == 0x3d)
      {
         test = EF2 == 0;
      }
      if (opcode == 0x3e)
      {
         test = EF3 == 0;
      }
      if (opcode == 0x3f)
      {
         test = EF4 == 0;
      }

      if (test)
      {
         int byte = 0;
         uint16_t rp_0 = M[R[P]];

         R[P] &= 0xff << (1 - byte) * 8;
         R[P] |= (uint16_t)rp_0 << byte * 8;
      }
      else
      {
         ++R[P];
      }
   }
   else if (0xc4 == opcode)
   {
      // C4    CONTINUE                                                                                               //  C4 NO OPERATION                            NOP
   }
   else if (0xc8 == opcode)
   {
      // C8    R[P] + 2 -> R[P]                                                                                       //  C8 LONG SKIP                               LSKP
      AS(R[P] + 2, R[P]);
   }
   else if (0x0c == I)
   {
      enum
      {
         CONTINUE,
         LONG_BRANCH,
         LONG_SKIP,
      };
      int action = CONTINUE;

      // C0    M[R[P]] -> R[P].1, M[R[P] + 1] -> R[P].0                                                               //  C0 LONG BRANCH                             LBR
      // C1    IF Q == 1,   M[R[P]] -> R[P].1, M[R[P] + 1] -> R[P].0, ELSE R[P] + 2 -> R[P]                           //  C1 LONG BRANCH IF Q == 1                   LBQ
      // C2    IF D == 0,   M[R[P]] -> R[P].1, M[R[P] + 1] -> R[P].0, ELSE R[P] + 2 -> R[P]                           //  C2 LONG BRANCH IF D == 0                   LBZ
      // C3    IF DF == 1,  M[R[P]] -> R[P].1, M[R[P] + 1] -> R[P].0, ELSE R[P] + 2 -> R[P]                           //  C3 LONG BRANCH IF DF == 1                  LBDF
      //
      // C5    IF Q == 0,   R[P] + 2 -> R[P], ELSE CONTINUE                                                           //  C5 LONG SKIP IF Q == 0                     LSNQ
      // C6    IF D != 0,   R[P] + 2 -> R[P], ELSE CONTINUE                                                           //  C6 LONG SKIP IF D != 0                     LSNZ
      // C7    IF DF == 0,  R[P] + 2 -> R[P], ELSE CONTINUE                                                           //  C7 LONG SKIP IF DF == 0                    LSNF
      //
      // C9    IF Q == 0,   M[R[P]] -> R[P].1, M[R[P] + 1] -> R[P].0, ELSE R[P] + 2 -> R[P]                           //  C9 LONG BRANCH IF Q == 0                   LBNQ
      // CA    IF D != 0,   M[R[P]] -> R[P].1, M[R[P] + 1] -> R[P].0, ELSE R[P] + 2 -> R[P]                           //  CA LONG BRANCH IF D != 0                   LBNZ
      // CB    IF DF == 0,  M[R[P]] -> R[P].1, M[R[P] + 1] -> R[P].0, ELSE R[P] + 2 -> R[P]                           //  CB LONG BRANCH IF DF == 0                  LBNF
      // CC    IF IE == 1,  R[P] + 2 -> R[P], ELSE CONTINUE                                                           //  CC LONG SKIP IF IE == 1                    LSIE
      // CD    IF Q == 1,   R[P] + 2 -> R[P], ELSE CONTINUE                                                           //  CD LONG SKIP IF Q == 1                     LSQ
      // CE    IF D == 0,   R[P] + 2 -> R[P], ELSE CONTINUE                                                           //  CE LONG SKIP IF D == 0                     LSZ
      // CF    IF DF == 1,  R[P] + 2 -> R[P], ELSE CONTINUE                                                           //  CF LONG SKIP IF DF == 1                    LSDF

      if (opcode == 0xc0)
      {
         action = LONG_BRANCH;
      }

      if (opcode == 0xc1)
      {
         action = Q == 1 ? LONG_BRANCH : LONG_SKIP;
      }
      if (opcode == 0xc2)
      {
         action = D == 0 ? LONG_BRANCH : LONG_SKIP;
      }
      if (opcode == 0xc3)
      {
         action = DF == 1 ? LONG_BRANCH : LONG_SKIP;
      }

      if (opcode == 0xc5)
      {
         action = Q == 0 ? LONG_SKIP : CONTINUE;
      }
      if (opcode == 0xc6)
      {
         action = D != 0 ? LONG_SKIP : CONTINUE;
      }
      if (opcode == 0xc7)
      {
         action = DF == 0 ? LONG_SKIP : CONTINUE;
      }

      if (opcode == 0xc9)
      {
         action = Q == 0 ? LONG_BRANCH : LONG_SKIP;
      }
      if (opcode == 0xca)
      {
         action = D != 0 ? LONG_BRANCH : LONG_SKIP;
      }
      if (opcode == 0xcb)
      {
         action = DF == 0 ? LONG_BRANCH : LONG_SKIP;
      }
      if (opcode == 0xcc)
      {
         action = IE == 1 ? LONG_SKIP : CONTINUE;
      }
      if (opcode == 0xcd)
      {
         action = Q == 1 ? LONG_SKIP : CONTINUE;
      }
      if (opcode == 0xce)
      {
         action = D == 0 ? LONG_SKIP : CONTINUE;
      }
      if (opcode == 0xcf)
      {
         action = DF == 1 ? LONG_SKIP : CONTINUE;
      }


      if (action == CONTINUE)
      {
      }
      else if (action == LONG_BRANCH)
      {
         uint16_t addr = M[0xffff & (R[P] + 0)] << 8 * 1 | M[0xffff & (R[P] + 1)] << 8 * 0;
         R[P] = addr;
      }
      else if (action == LONG_SKIP)
      {
         AS(R[P] + 2, R[P]);
      }
   }
   else if (0x68 == opcode)
   {
      // INVALID
   }
   else if (0x60 == opcode)
   {
      // 60    R[X] + 1 -> R[X]                                                                                       //  60 INCREMENT REG X                         IRX
      AS(R[X] + 1, R[X]);
   }
   else if (0x06 == I)
   {
      if (N >= 0x01 && N <= 0x07)
      {
         // 61    M[R[X]] -> BUS; R[X] + 1 -> R[X]; N LINES = 1                                                          //  61 OUTPUT 1                                OUT 1
         // 62    M[R[X]] -> BUS; R[X] + 1 -> R[X]; N LINES = 2                                                          //  62 OUTPUT 2                                OUT 2
         // 63    M[R[X]] -> BUS; R[X] + 1 -> R[X]; N LINES = 3                                                          //  63 OUTPUT 3                                OUT 3
         // 64    M[R[X]] -> BUS; R[X] + 1 -> R[X]; N LINES = 4                                                          //  64 OUTPUT 4                                OUT 4
         // 65    M[R[X]] -> BUS; R[X] + 1 -> R[X]; N LINES = 5                                                          //  65 OUTPUT 5                                OUT 5
         // 66    M[R[X]] -> BUS; R[X] + 1 -> R[X]; N LINES = 6                                                          //  66 OUTPUT 6                                OUT 6
         // 67    M[R[X]] -> BUS; R[X] + 1 -> R[X]; N LINES = 7                                                          //  67 OUTPUT 7                                OUT 7
         outp(N - 0, M[R[X]]);
         AS(R[X] + 1, R[X]);
      }
      else if (N >= 0x09 && N <= 0x0f)
      {
         uint8_t val = inp(N - 8);

         // 69    BUS -> M[R[X]]; BUS -> D; N LINES = 1                                                                  //  69 INPUT 1                                 INP 1
         // 6A    BUS -> M[R[X]]; BUS -> D; N LINES = 2                                                                  //  6A INPUT 2                                 INP 2
         // 6B    BUS -> M[R[X]]; BUS -> D; N LINES = 3                                                                  //  6B INPUT 3                                 INP 3
         // 6C    BUS -> M[R[X]]; BUS -> D; N LINES = 4                                                                  //  6C INPUT 4                                 INP 4
         // 6D    BUS -> M[R[X]]; BUS -> D; N LINES = 5                                                                  //  6D INPUT 5                                 INP 5
         // 6E    BUS -> M[R[X]]; BUS -> D; N LINES = 6                                                                  //  6E INPUT 6                                 INP 6
         // 6F    BUS -> M[R[X]]; BUS -> D; N LINES = 7                                                                  //  6F INPUT 7                                 INP 7
         AS(val, M[R[X]]);
         AS(val, D);
      }
   }
   else if (0x70 == opcode)
   {
      // 70    M[R[X]] -> [X, P]; R[X] + 1 -> R[X], 1 -> IE                                                           //  70 RETURN                                  RET
      uint8_t val = 0;

      AS(M[R[X]], val);
      AS(val >> 0 & 0xf, P);
      AS(val >> 4 & 0xf, X);
      AS(R[X] + 1, R[X]);
      AS(0, IE);
   }
   else if (0x71 == opcode)
   {
      // 71    M[R[X]] -> [X, P]; R[X] + 1 -> R[X], 0 -> IE                                                           //  71 DISABLE                                 DIS
      uint8_t val = 0;

      AS(M[R[X]], val);
      AS(val >> 0 & 0xf, P);
      AS(val >> 4 & 0xf, X);
      AS(R[X] + 1, R[X]);
      AS(1, IE);
   }
   else if (0x72 == opcode)
   {
      // 72    M[R[X]] -> D; R[X] + 1 -> R[X]                                                                         //  72 LOAD VIA X AND ADVANCE                  LDXA
      AS(M[R[X]], D);
      AS(R[X] + 1, R[X]);
   }
   else if (0x73 == opcode)
   {
      // 73    D -> M[R[X]]; R[X] - 1 -> R[X]                                                                         //  73 STORE VIA X AND DECREMENT               STXD
      AS(D, M[R[X]]);
      AS(R[X] - 1, R[X]);
   }
   else if (0x74 == opcode)
   {
      // 74    M[R[X]] + D + DF -> DF, D                                                                              //  74 ADD WITH CARRY                          ADC
      uint16_t val = M[R[X]] + D + DF;

      AS(val, D);
      AS(!!(val >> 8), DF);
   }
   else if (0x75 == opcode)
   {
      // 75    M[R[X]] - D - [NOT DF] -> DF, D                                                                        //  75 SUBTRACT D WITH BORROW                  SDB
      uint16_t val = M[R[X]] - D - !DF;

      AS(val, D);
      AS(!!!(val >> 8), DF);
   }
   else if (0x76 == opcode)
   {
      int df = DF;

      // 76    SHIFT D RIGHT, LSB[D] -> DF, DF -> MSB[D]                                                              //  76 SHIFT RIGHT WITH CARRY                  SHRC
      AS(!!(D & 1), DF);
      AS(D >> 1, D);
      AS(D | df << 7, D);
   }
   else if (0x77 == opcode)
   {
      // 77    D - M[R[X]] - [NOT DF] -> DF, D                                                                        //  77 SUBTRACT MEMORY WITH BORROW             SMB
      uint16_t val = D - M[R[X]] - !DF;

      AS(val, D);
      AS(!!!(val >> 8), DF);
   }
   else if (0x78 == opcode)
   {
      // 78    T -> M[R[X]]                                                                                           //  78 SAVE                                    SAV
      AS(T, M[R[X]]);
   }
   else if (0x79 == opcode)
   {
      // 79    [X, P] -> T; [X, P] -> M[R[2]], THEN P -> X; R[2] - 1 -> R[2]                                          //  79 PUSH X, P TO STACK                      MARK
      uint8_t xp = (X & 0xf) << 4 * 1 | (P & 0xf) << 4 * 0;

      AS(xp, T);
      AS(xp, M[R[2]]);
      AS(P, X);
      AS(R[2] - 1, R[2]);
   }
   else if (0x7a == opcode)
   {
      // 7A    0 -> Q                                                                                                 //  7A RESET Q                                 REQ
      AS(0, Q);
   }
   else if (0x7b == opcode)
   {
      // 7B    1 -> Q                                                                                                 //  7B SET Q                                   SEQ
      AS(1, Q);
   }
   else if (0x7c == opcode)
   {
      // 7C    M[R[P]] + D + DF -> DF, D; R[P] + 1 -> R[P]                                                            //  7C ADD WITH CARRY, IMMEDIATE               ADCI
      uint16_t val = M[R[P]] + D + DF;

      AS(val, D);
      AS(!!(val >> 8), DF);
      AS(R[P] + 1, R[P]);
   }
   else if (0x7d == opcode)
   {
      // 7D    M[R[P]] - D - [NOT DF] -> DF, D; R[P] + 1 -> R[P]                                                      //  7D SUBTRACT D WITH BORROW, IMMEDIATE       SDBI
      uint16_t val = M[R[P]] - D - !DF;

      AS(val, D);
      AS(!!!(val >> 8), DF);
      AS(R[P] + 1, R[P]);
   }
   else if (0x7e == opcode)
   {
      int df = DF;

      // 7E    SHIFT D LEFT, MSB[D] -> DF, DF -> LSB[D]                                                               //  7E SHIFT LEFT WITH CARRY                   SHLC
      AS(!!(D & 1 << 7), DF);
      AS(D << 1, D);
      AS(D | df, D);
   }
   else if (0x7f == opcode)
   {
      // 7F    D - M[R[P]] - [NOT DF] -> DF, D; R[P] + 1 -> R[P]                                                      //  7F SUBTRACT MEMORY WITH BORROW, IMMEDIATE  SMBI
      uint16_t val = D - M[R[P]] - !DF;

      AS(val, D);
      AS(!!!(val >> 8), DF);
      AS(R[P] + 1, R[P]);
   }
   else if (0xf6 == opcode)
   {
      // F6    SHIFT D RIGHT, LSB[D] -> DF, 0 -> MSB[D]                                                               //  F6 SHIFT RIGHT                             SHR
      AS(!!(D & 1), DF);
      AS(D >> 1, D);
   }
   else if (0xfe == opcode)
   {
      // FE    SHIFT D LEFT, MSB[D] -> DF, 0 -> LSB[D]                                                                //  FE SHIFT LEFT                              SHL
      AS(!!(D & 1 << 7), DF);
      AS(D << 1, D);
   }
   else if (0xf == I)
   {
      int imm = !!(N & 0x8);
      int xp = imm ? P : X;

      switch (N & 0x7)
      {
         case 0:
            // F0    M[R[X]] -> D                                                                                           //  F0 LOAD VIA X                              LDX
            // F8    M[R[P]] -> D; R[P] + 1 -> R[P]                                                                         //  F8 LOAD IMMEDIATE                          LDI
            AS(M[R[xp]], D);
            break;
         case 1:
            // F1    M[R[X]] OR D -> D                                                                                      //  F1 OR                                      OR
            // F9    M[R[P]] OR D -> D; R[P] + 1 -> R[P]                                                                    //  F9 OR IMMEDIATE                            ORI
            AS(M[R[xp]] | D, D);
            break;
         case 2:
            // F2    M[R[X]] AND D -> D                                                                                     //  F2 AND                                     AND
            // FA    M[R[P]] AND D -> D; R[P] + 1 -> R[P]                                                                   //  FA AND IMMEDIATE                           ANI
            AS(M[R[xp]] & D, D);
            break;
         case 3:
            // F3    M[R[X]] XOR D -> D                                                                                     //  F3 EXCLUSIVE OR                            XOR
            // FB    M[R[P]] XOR D -> D; R[P] + 1 -> R[P]                                                                   //  FB EXCLUSIVE OR IMMEDIATE                  XRI
            AS(M[R[xp]] ^ D, D);
            break;
         case 4:
            // F4    M[R[X]] + D -> DF, D                                                                                   //  F4 ADD                                     ADD
            // FC    M[R[P]] + D -> DF, D; R[P] + 1 -> R[P]                                                                 //  FC ADD IMMEDIATE                           ADI
            {
               uint16_t val = M[R[xp]] + D;

               AS(val, D);
               AS(!!(val >> 8), DF);
            }
            break;
         case 5:
            // F5    M[R[X]] - D -> DF, D                                                                                   //  F5 SUBTRACT D                              SD
            // FD    M[R[P]] - D -> DF, D; R[P] + 1 -> R[P]                                                                 //  FD SUBTRACT D IMMEDIATE                    SDI
            {
               uint16_t val = M[R[xp]] - D;

               AS(val, D);
               AS(!!!(val >> 8), DF);
            }
            break;
         case 7:
            // F7    D - M[R[X]] -> DF, D                                                                                   //  F7 SUBTRACT MEMORY                         SM
            // FF    D - M[R[P]] -> DF, D; R[P] + 1 -> R[P]                                                                 //  FF SUBTRACT MEMORY IMMEDIATE               SMI
            {
               uint16_t val = D - M[R[xp]];

               AS(val, D);
               AS(!!!(val >> 8), DF);
            }
            break;
      }

      if (imm)
      {
         AS(R[P] + 1, R[P]);
      }
   }

   return 1;
}
