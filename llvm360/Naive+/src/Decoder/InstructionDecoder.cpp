#include "InstructionDecoder.h"
#include "../../common/Debug.h"

// Instruction Bit Field
class IBF {
public:
  IBF(const uint32_t instr)
    : m_instr(instr) {
  }

  uint32_t GetAt(uint32_t bestart,
    const uint32_t length,
    const bool signExtend = false,
    const int shift = 0) {
    uint32_t start = (32 - bestart) - length;
    const uint32_t mask = ((1 << length) - 1);
    uint32_t bitSize = length;
    uint32_t ext = (m_instr >> start) & mask;

    // additional left shift
    if (shift > 0) {
      ext <<= shift;
      bitSize += shift;
    }

    // if signed expand copy the sign bit
    if (signExtend) {
      const uint32_t signBitValue = ext & (1 << (bitSize - 1));
      if (signBitValue != 0) {
        const uint32_t signMask = ((uint32_t)-1) << bitSize;
        ext |= signMask;
      }
    }

    // right shift (always do that after sign conversion)
    if (shift < 0) {
      ext >>= (-shift);
    }

    // save value
    return ext;
  }

  inline const uint32_t GetInstr() const {
    return m_instr;
  }

private:
  uint32_t m_instr = 0;
};

InstructionDecoder::InstructionDecoder(const Section *imageSection) {
  m_imageSection = imageSection;
  m_imageBaseAddress = imageSection->GetVirtualOffset();
  m_imageDataSize = imageSection->GetVirtualSize();
  m_imageDataPtr = (const uint8_t *)imageSection->GetImage()->GetMemory() +
                   (m_imageBaseAddress - imageSection->GetImage()->GetBaseAddress());
}

uint32_t InstructionDecoder::GetInstructionAt(uint32_t address, Instruction &instruction) {
  // build input stream
  const uint64_t offset = address - m_imageBaseAddress;
  const uint8_t *stride = m_imageDataPtr + offset;

  return DecodeInstruction(stride, instruction);
}

static inline void SetInstr(Instruction &inst,
  std::string name,
  const std::vector<uint32_t> &operands) {
  inst.opcName = name;
  inst.ops = operands; // Use the raw pointer if required
}

#define INST(name, ...)                                                                            \
  {                                                                                                \
    SetInstr(instruction, name, std::vector<uint32_t>{__VA_ARGS__});                               \
    return 4;                                                                                      \
  }

// https://www.nxp.com/docs/en/user-guide/MPCFPE_AD_R1.pdf
//
// *Stride* pointer to start of the 4 bytes of the instruction in memory
// [out] instruction - the decoded instruction
uint32_t InstructionDecoder::DecodeInstruction(const uint8_t *stride, Instruction &instruction) {
    // Xenons PowerPC instructions are always 4 bytes long,
    // this helps us to avoid instruction length decoding since
    // we already know the length
    // we can store it in a uint32 and use bitfields to extract the opcode and operands

    // cause how it is stored in the stride we have to swap endians
  uint32_t instr = SwapInstrBytes(*(uint32_t *)stride);
    instruction.address = m_imageBaseAddress + (stride - m_imageDataPtr);
    instruction.instrWord = instr;
  IBF *ibf = new IBF(instr);

    uint32_t opcode = ibf->GetAt(0, 6);
    uint32_t ext21_10 = ibf->GetAt(21, 10);
    uint32_t ext27_3 = ibf->GetAt(27, 3);
    uint32_t ext22_9 = ibf->GetAt(22, 9);
    uint32_t S = ibf->GetAt(6, 5);
    uint32_t aa = ibf->GetAt(30, 1);
    uint32_t b31_1 = ibf->GetAt(31, 1);
    uint32_t b11_10 = ibf->GetAt(11, 10);
    uint32_t spr5 = ibf->GetAt(11, 5);
    uint32_t li = ibf->GetAt(6, 24, true, 2);
    uint32_t ext26_5 = ibf->GetAt(26, 5);

    uint32_t b11_5 = ibf->GetAt(11, 5);
    uint32_t b9_2 = ibf->GetAt(9, 2);
    uint32_t b6_5 = ibf->GetAt(6, 5);
    uint32_t b12_8 = ibf->GetAt(12, 8);
    uint32_t b7_8 = ibf->GetAt(7, 8);
    uint32_t b11_1 = ibf->GetAt(11, 1);
    uint32_t b16_16 = ibf->GetAt(16, 16);
    uint32_t b16_14 = ibf->GetAt(16, 14);
    uint32_t b16_5 = ibf->GetAt(16, 5);
    uint32_t b6_3 = ibf->GetAt(6, 3);
    uint32_t b10_1 = ibf->GetAt(10, 1);
    uint32_t b21_5 = ibf->GetAt(21, 5);
    uint32_t b26_1 = ibf->GetAt(26, 1);
    uint32_t b21_1 = ibf->GetAt(21, 1);
    uint32_t b26_5 = ibf->GetAt(26, 5);

    uint32_t vshift28 = ibf->GetAt(28, 2, false, 5);
    uint32_t vshift30 = ibf->GetAt(30, 2, false, 5);
    uint32_t vshift26a = ibf->GetAt(21, 1, false, 6);
    uint32_t vshift26b = ibf->GetAt(26, 1, false, 5);

    // extended regs (full 128 regs)
    uint32_t vreg6 = vshift28 + b6_5;
    uint32_t vreg11 = vshift26a + vshift26b + b11_5;
    uint32_t vreg16 = vshift30 + b16_5;

    // The funny giant switch statement instruction decoder
    switch (opcode) {

    case 0:
        INST("nop")

    case 2:
        INST("tdi", b6_5, b11_5, b16_16); // tdi TO,rA,SIMM
    case 3:
        INST("twi", b6_5, b11_5, b16_16); // twi TO,rA,SIMM

        // VMX fun
    case 4: {
        uint32_t v21_11 = ibf->GetAt(21, 11);
        uint32_t v26_6 = ibf->GetAt(26, 6);
        uint32_t v21_7 = ibf->GetAt(21, 7);
        uint32_t v21_1 = ibf->GetAt(21, 1);
        uint32_t v22_4 = ibf->GetAt(22, 4);

        switch (v21_11) {
        case 199:
            INST("stvewx", S, b11_5, b16_5) // stvewx vS,rA,rB

        case 1156: {
                if (b11_5 == b16_5) {
                    INST("mr", b6_5, b11_5);
                }
                else {
                    INST("vor", b6_5, b11_5, b16_5);
                }
            }
        }

        // extended
        switch (v21_7) {
            // 128 ?!?!?!!?!?

        case 64:
            INST("lvlx", vreg6, b11_5, b16_5)
        case 96:
            INST("lvlxl", vreg6, b11_5, b16_5)
        case 68:
            INST("lvrx", vreg6, b11_5, b16_5)
        case 100:
            INST("lvrxl", vreg6, b11_5, b16_5)
        case 12:
            INST("lvx", vreg6, b11_5, b16_5)
        case 44:
            INST("lvxl", vreg6, b11_5, b16_5)

        case 24:
            INST("stvewx", vreg6, b11_5, b16_5)
        case 80:
            INST("stvlx", vreg6, b11_5, b16_5)
        case 112:
            INST("stvlxl", vreg6, b11_5, b16_5)
        case 84:
            INST("stvrx", vreg6, b11_5, b16_5)

                // stvx vS,rA,rB
        case 28:
            INST("stvx128", S, b11_5, b16_5) // stvx128 vS,rA,rB
        }

        switch (v26_6) {
        case 46:
            INST("vmaddfp", b6_5, b11_5, b21_5, b16_5);
        case 47:
            INST("vnmsubfp", b6_5, b11_5, b16_5, b21_5);
        }

        switch (v21_1) {
            //// vsldoi128
        case 0:
            INST("vsldoi", vreg6, vreg11, vreg16, v22_4);
        }

        printf("VMX OPS NOT IMPLEMENTED V21=%d, V26=%d, V21_7=%d opcode: %d\n",
            v21_11,
            v26_6,
            v21_7,
            opcode);
        return 0;
    }

          // EXT VMX
    case 5: {
        uint32_t v22_4 = ibf->GetAt(22, 4);
        uint32_t v27_1 = ibf->GetAt(27, 1);

        if (v27_1 == 0) {

            switch (v22_4) {
            case 0:
                INST("vperm", vreg6, vreg11, vreg16, 0);
            case 1:
                INST("vperm", vreg6, vreg11, vreg16, 1);
            case 2:
                INST("vperm", vreg6, vreg11, vreg16, 2);
            case 3:
                INST("vperm", vreg6, vreg11, vreg16, 3);
            case 4:
                INST("vperm", vreg6, vreg11, vreg16, 4);
            case 5:
                INST("vperm", vreg6, vreg11, vreg16, 5);
            case 6:
                INST("vperm", vreg6, vreg11, vreg16, 6);
            case 7:
                INST("vperm", vreg6, vreg11, vreg16, 7);

            case 8:
                INST("vpkshss", vreg6, vreg11, vreg16);
            case 9:
                INST("vpkshus", vreg6, vreg11, vreg16);
            case 10:
                INST("vpkswss", vreg6, vreg11, vreg16);
            case 11:
                INST("vpkswus", vreg6, vreg11, vreg16);
            case 12:
                INST("vpkuhum", vreg6, vreg11, vreg16);
            case 13:
                INST("vpkuhus", vreg6, vreg11, vreg16);
            case 14:
                INST("vpkuwum", vreg6, vreg11, vreg16);
            case 15:
                INST("vpkuwus", vreg6, vreg11, vreg16);
            }
        }
        else {

            switch (v22_4) {

            case 0:
                INST("vaddfp", vreg6, vreg11, vreg16);
            case 1:
                INST("vsubfp", vreg6, vreg11, vreg16);
            case 2:
                INST("vmulfp128", vreg6, vreg11, vreg16);

            case 3:
                INST("vmaddfp", vreg6, vreg11, vreg16, vreg6);
            case 4:
                INST("vmaddfp", vreg6, vreg11, vreg6, vreg16); // addc

            case 5:
                INST("vnmsubfp", vreg6, vreg11, vreg16, vreg6);

            case 6:
                INST("vdot3fp", vreg6, vreg11, vreg16);
            case 7:
                INST("vdot4fp", vreg6, vreg11, vreg16);

            case 8:
                INST("vand", vreg6, vreg11, vreg16);
            case 9:
                INST("vandc", vreg6, vreg11, vreg16);
            case 10:
                INST("vnor", vreg6, vreg11, vreg16);
            case 12:
                INST("vxor", vreg6, vreg11, vreg16);
            case 13:
                INST("vsel", vreg6, vreg11, vreg16, vreg6); // arg3 == arg0
            case 11: {
                if (vreg11 == vreg16) {
                    INST("mr", vreg6, vreg11);
                }
                else {

                    INST("vor", vreg6, vreg11, vreg16);
                }
            }
            }
        }

        printf("VMX OPS NOT IMPLEMENTED V22=%d, V27=%d, opcode: %d\n", v22_4, v27_1, opcode);
        return 0;
    }

          // EXT VMX
    case 6: {
        uint32_t v21_11 = ibf->GetAt(21, 11);
        uint32_t v26_6 = ibf->GetAt(26, 6);
        uint32_t v21_7 = ibf->GetAt(21, 7);
        uint32_t v22_4 = ibf->GetAt(22, 4);
        uint32_t v27_1 = ibf->GetAt(27, 1);

        switch (v21_7) {

        case (33 + (0 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 0 * 32);
        case (33 + (1 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 1 * 32);
        case (33 + (2 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 2 * 32);
        case (33 + (3 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 3 * 32);
        case (33 + (4 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 4 * 32);
        case (33 + (5 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 5 * 32);
        case (33 + (6 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 6 * 32);
        case (33 + (7 * 4)):
            INST("vpermwi128", vreg6, vreg16, b11_5 + 7 * 32);

            // PACKED
        case 97:
            INST("vpkd3d128", vreg6, vreg16, (uint32_t)(b11_5 / 4), (uint32_t)(b11_5 & 3), (uint32_t)(0))
        case 101:
            INST("vpkd3d128", vreg6, vreg16, (uint32_t)(b11_5 / 4), (uint32_t)(b11_5 & 3), (uint32_t)(1))
        case 105:
            INST("vpkd3d128", vreg6, vreg16, (uint32_t)(b11_5 / 4), (uint32_t)(b11_5 & 3), (uint32_t)(2))
        case 109:
            INST("vpkd3d128", vreg6, vreg16, (uint32_t)(b11_5 / 4), (uint32_t)(b11_5 & 3), (uint32_t)(3))

        case 111:
            INST("vlogefp", vreg6, vreg16)
        case 107:
            INST("vexptefp", vreg6, vreg16)

        case 35:
            INST("vcfpsxws", vreg6, vreg16, b11_5);
        case 39:
            INST("vcfpuxws", vreg6, vreg16, b11_5);

            // TODO, make those right
            // vspltisw128
        case 119:
            INST("vspltisw", vreg6, b11_5) // 128?
        case 43:
            INST("vcsxwfp", vreg6, vreg16, b11_5)

        case 103:
            INST("vrsqrtefp", vreg6, vreg16)
        }

        if (v27_1 == 0) {
            switch (v22_4) {
            case 0:
                INST("vcmpeqfp", vreg6, vreg11, vreg16)
            case 1:
                INST("vcmpeqfpRC", vreg6, vreg11, vreg16)
            case 2:
                INST("vcmpgefp", vreg6, vreg11, vreg16)
            case 3:
                INST("vcmpgefpRC", vreg6, vreg11, vreg16)
            case 4:
                INST("vcmpgtfp", vreg6, vreg11, vreg16)
            case 5:
                INST("vcmpgtfpRC", vreg6, vreg11, vreg16)
            case 6:
                INST("vcmpbfp", vreg6, vreg11, vreg16)
            case 7:
                INST("vcmpbfpRC", vreg6, vreg11, vreg16)
            case 8:
                INST("vcmpequw", vreg6, vreg11, vreg16)
            case 9:
                INST("vcmpequwRC", vreg6, vreg11, vreg16)
            case 10:
                INST("vmaxfp", vreg6, vreg11, vreg16)
            case 11:
                INST("vminfp", vreg6, vreg11, vreg16)
            case 12:
                INST("vmrghw", vreg6, vreg11, vreg16)
            case 13:
                INST("vmrglw", vreg6, vreg11, vreg16)
            }
        }
        else {
            switch (v22_4) {
            case 1:
                INST("vrlw", vreg6, vreg11, vreg16);

            case 3:
                INST("vslw", vreg6, vreg11, vreg16);

            case 5:
                INST("vsraw", vreg6, vreg11, vreg16);

            case 7:
                INST("vsrw", vreg6, vreg11, vreg16);

            case 12:
                INST("vrfim", vreg6, vreg16);
            case 13:
                INST("vrfin", vreg6, vreg16);
            case 14:
                INST("vrfip", vreg6, vreg16);
            case 15:
                INST("vrfiz", vreg6, vreg16);
            }
        }

        printf("VMX OPS NOT IMPLEMENTED V22_4=%d, V27_1=%d, V21_7=%d opcode: %d\n",
            v22_4,
            v27_1,
            v21_7,
            opcode);
        return 0;
    }

          // muli (multiply with immediate)
    case 7:
        INST("mulli", S, b11_5, b16_16);

        // subf( subtract from)
    case 8:
        INST("subfic", S, b11_5, b16_16);

    case 10: {
        uint32_t l = ibf->GetAt(10, 1);
        if (!l)
            INST("cmplwi", b6_3, 0, b11_5, b16_16)
            INST("cmpldi", b6_3, 1, b11_5, b16_16)
    }

           // cmpi (cmpdi, cmpwi)
    case 11: {
        uint32_t l = ibf->GetAt(10, 1);
        if (!l)
            INST("cmpwi", b6_3, b10_1, b11_5, b16_16)
            INST("cmpdi", b6_3, b10_1, b11_5, b16_16)
    }
    case 12:
        INST("addic", S, b11_5, b16_16); // addic rD,rA,SIMM
    case 13:
        INST("addicRC", S, b11_5, b16_16); // addic. rD,rA,SIMM


    // li and lis are just addi and addis instructions
	// i'm adding simplified mnemonics just for the sake of it
    case 14: {
        if (b11_5 == 0){
            INST("li", S, 0, b16_16)        // addi rD,0,SIMM
        }  
        INST("addi", S, b11_5, b16_16) // addi rD,rA,SIMM
    }

    case 15: {
        if (b11_5 == 0) {
            INST("lis", S, 0, b16_16)        // addis rD,0,SIMM
        }
        INST("addis", S, b11_5, b16_16) // addis rD,rA,SIMM
    }

           // bc, bca, bcl, bcla (BO,BI,target_addr)
    case 16:
        if (!aa && !b31_1)
            INST("bc", S, b11_5, b16_14);
        if (!aa && b31_1)
            INST("bcl", S, b11_5, b16_14);
        if (aa && !b31_1)
            INST("bca", S, b11_5, b16_14);
        INST("bcla", S, b11_5, b16_14);

        // b,ba,bl,bla
    case 18:
        if (!aa && !b31_1)
            INST("b", li);
        if (!aa && b31_1)
            INST("bl", li)
            if (aa && !b31_1)
                INST("ba", li);
        INST("bla", li)

    case 19: {
            switch (ext21_10) {
            case 16:
                if (b31_1)
                    INST("bclrl", b6_5, b11_5)
                    INST("bclr", b6_5, b11_5)

            case 528:
                if (b31_1)
                    INST("bcctrl", b6_5, b11_5)
                    INST("bcctr", b6_5, b11_5)
            }

            printf("EXTENDED OPS NOT IMPLEMENTED EXTOC21: %i EXTOC21_9: %i OP: %i\n",
                ext21_10,
                ext22_9,
                opcode);
            return 0;
        }

        // rlwimi, rlwimi., rlwinm, rlwinm., rlwnm, rlwnm.
    case 20:
        if (b31_1)
            INST("rlwimiRC", b11_5, S, b16_5, b21_5, b26_5)
            INST("rlwimi", b11_5, S, b16_5, b21_5, b26_5)

    case 21:
        if (b31_1)
            INST("rlwinmRC", b11_5, S, b16_5, b21_5, b26_5)
            INST("rlwinm", b11_5, S, b16_5, b21_5, b26_5)

    case 23:
        if (b31_1)
            INST("rlwnmRC", b11_5, S, b16_5, b21_5, b26_5)
            INST("rlwnm", b11_5, S, b16_5, b21_5, b26_5)

    case 25:
        INST("oris", b11_5, S, b16_16);
    case 26:
        INST("xori", b11_5, S, b16_16);
    case 27:
        INST("xoris", b11_5, S, b16_16);
    case 28:
        INST("andiRC", b11_5, S, b16_16);
    case 29:
        INST("andisRC", b11_5, S, b16_16);

        // ori rA,rS,UIMM
    case 24:
        INST("ori", b11_5, b6_5, b16_16);

        // Shift Stuff
    case 30: {

        switch (ext27_3) {
            // rldicl, rldicl., rldicr, rldicr., rldic, rldic., rldimi, rldimi.
        case 0:
            if (b31_1)
                INST("rldiclRC", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))
                INST("rldicl", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))

        case 1:
            if (b31_1)
                INST("rldicrlRC", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))
                INST("rldicr", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))

        case 2:
            if (b31_1)
                INST("rldicRC", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))
                INST("rldic", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))

        case 3:
            if (b31_1)
                INST("rldimiRC", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))
                INST("rldimi", b11_5, S, b16_5 + (aa ? 32 : 0), b21_5 + (b26_1 ? 32 : 0))
        }

        uint32_t ext27_4 = ibf->GetAt(27, 4);
        switch (ext27_4) {
            // rldcl, rldcl., rldcr, rldcr.
        case 8:
            if (b31_1)
                INST("rldclRC", b11_5, S, b16_5, b21_5 + (b26_1 ? 32 : 0))
                INST("rldcl", b11_5, S, b16_5, b21_5 + (b26_1 ? 32 : 0))

        case 9:
            if (b31_1)
                INST("rldcrRC", b11_5, S, b16_5, b21_5 + (b26_1 ? 32 : 0))
                INST("rldcr", b11_5, S, b16_5, b21_5 + (b26_1 ? 32 : 0))
        }

        printf("EXTENDED OPS NOT IMPLEMENTED EXTOC27_4: %i OP: %i\n", ext27_4, opcode);
        return 0;
    }

    case 31: {
        switch (ext21_10) {
            // cmpw, cmpd
        case 0: {
            uint32_t l = ibf->GetAt(10, 1);
            if (!l)
                INST("cmpw", b6_3, b10_1, b11_5, b16_5)
                INST("cmpd", b6_3, b10_1, b11_5, b16_5)
        }

              // mftb (move from time base register)
        case 371:
            INST("mftb", S, b11_10);

        case 21:
            INST("ldx", S, b11_5, )
        case 53:
            INST("ldux", S, b11_5, b16_5)

                // lwzx, lwzux, lwax, lwaux
        case 23:
            INST("lwzx", S, b11_5, b16_5)
        case 55:
            INST("lwzux", S, b11_5, b16_5)
        case 341:
            INST("lwax", S, b11_5, b16_5)
        case 373:
            INST("lwaux", S, b11_5, b16_5)

                // lhzx, lhzux, lhax, lhaux
        case 279:
            INST("lhzx", S, b11_5, b16_5)
        case 311:
            INST("lhzux", S, b11_5, b16_5)
        case 343:
            INST("lhax", S, b11_5, b16_5)
        case 375:
            INST("lhaux", S, b11_5, b16_5)

        case 790:
            INST("lhbrx", S, b11_5, b16_5)
        case 534:
            INST("lwbrx", S, b11_5, b16_5)
        case 918:
            INST("sthbrx", S, b11_5, b16_5)
        case 662:
            INST("stwbrx", S, b11_5, b16_5)

                // stfsx, stfsux, stfdx, stfdux, stfiwx
        case 663:
            INST("stfsx", S, b11_5, b16_5)
        case 695:
            INST("stfsux", S, b11_5, b16_5)
        case 727:
            INST("stfdx", S, b11_5, b16_5)
        case 759:
            INST("stfdux", S, b11_5, b16_5)
        case 983:
            INST("stfiwx", S, b11_5, b16_5)

                // stwx, stwux
        case 151:
            INST("stwx", S, b11_5, b16_5)
        case 183:
            INST("stwux", S, b11_5, b16_5)

                // sthx, sthux
        case 407:
            INST("sthx", S, b11_5, b16_5)
        case 439:
            INST("sthux", S, b11_5, b16_5)

                // stbx, stbux
        case 215:
            INST("stbx", S, b11_5, b16_5)
        case 247:
            INST("stbux", S, b11_5, b16_5)

                // lbzx, lbzxu
        case 87:
            INST("lbzx", S, b11_5, b16_5)
        case 119:
            INST("lbzux", S, b11_5, b16_5)

                // stdx, stdux
        case 149:
            INST("stdx", S, b11_5, b16_5)
        case 181:
            INST("stdux", S, b11_5, b16_5)

        case 122:
            INST("popcntb", b11_5, S, b6_5)

                // sync (memory bariers)
        case 598:
            if (b9_2 == 0)
                INST("sync");
            if (b9_2 == 1)
                INST("lwsync");
            if (b9_2 == 2)
                INST("ptesync");
            printf("Decode: Invalid sync instruction type\n");

            // eieio
        case 854:
            INST("eieio");

            // lfsx, lfsux, lfdx, lfdux
        case 535:
            INST("lfsx", S, b11_5, b16_5)
        case 567:
            INST("lfsux", S, b11_5, b16_5)
        case 599:
            INST("lfdx", S, b11_5, b16_5)
        case 631:
            INST("lfdux", S, b11_5, b16_5)

                // extsb, extsb., extsh, extsh., extsw, extsw., popcntb, popcntb., cntlzw, cntlzw., cntlzd,
                // cntlzd.
        case 954:
            if (b31_1)
                INST("extsbRC", b11_5, S, b6_5)
                INST("extsb", b11_5, S, b6_5)

        case 922:
            if (b31_1)
                INST("extshRC", b11_5, S, b6_5)
                INST("extsh", b11_5, S, b6_5)

        case 986:
            if (b31_1)
                INST("extswRC", b11_5, S, b6_5)
                INST("extsw", b11_5, S, b6_5)

        case 26:
            if (b31_1)
                INST("cntlzwRC", b11_5, S, b6_5)
                INST("cntlzw", b11_5, S, b6_5)

        case 58:
            if (b31_1)
                INST("cntlzdRC", b11_5, S, b6_5)
                INST("cntlzd", b11_5, S, b6_5)

                // sld, sld., slw, slw., srd, srd., srw, srw., srad, srad., sraw, sraw.
        case 27:
            if (b31_1)
                INST("sldRC", b11_5, S, b16_5)
                INST("sld", b11_5, S, b16_5)

        case 24:
            if (b31_1)
                INST("slwRC", b11_5, S, b16_5)
                INST("slw", b11_5, S, b16_5)

        case 539:
            if (b31_1)
                INST("srdRC", b11_5, S, b16_5)
                INST("srd", b11_5, S, b16_5)

        case 536:
            if (b31_1)
                INST("srwRC", b11_5, S, b16_5)
                INST("srw", b11_5, S, b16_5)

        case 794:
            if (b31_1)
                INST("sradRC", b11_5, S, b16_5)
                INST("srad", b11_5, S, b16_5)

        case 792:
            if (b31_1)
                INST("srawRC", b11_5, S, b16_5)
                INST("sraw", b11_5, S, b16_5)

                // and, and., or, or., xor, xor., nand, nand, nor, nor., eqv, eqv., andc, andc., orc, orc
        case 28:
            if (b31_1)
                INST("andRC", b11_5, S, b16_5)
                INST("and", b11_5, S, b16_5)

                // cmplw, cmpld
        case 32: {
                uint32_t l = ibf->GetAt(10, 1);
                if (!l)
                    INST("cmplw", b6_3, 0, b11_5, b16_5);
                INST("cmpld", b6_3, 1, b11_5, b16_5);
            }

            // or rA,rS,rB (also RC)
        case 444:
            if (b31_1)
                INST("orRC", b11_5, S, b16_5)
                INST("or", b11_5, S, b16_5)

        case 316:
            if (b31_1)
                INST("xorRC", b11_5, S, b16_5)
                INST("xor", b11_5, S, b16_5)

        case 476:
            if (b31_1)
                INST("nandRC", b11_5, S, b16_5)
                INST("nand", b11_5, S, b16_5)

        case 124:
            if (b31_1)
                INST("norRC", b11_5, S, b16_5)
                INST("nor", b11_5, S, b16_5)

        case 284:
            if (b31_1)
                INST("eqvRC", b11_5, S, b16_5)
                INST("eqv", b11_5, S, b16_5)

        case 60:
            if (b31_1)
                INST("andcRC", b11_5, S, b16_5)
                INST("andc", b11_5, S, b16_5)

        case 412:
            if (b31_1)
                INST("orcRC", b11_5, S, b16_5)
                INST("orc", b11_5, S, b16_5)

                // sradi, sradi., srawi, srawi.
        case 824:
            if (b31_1)
                INST("srawiRC", b11_5, S, b16_5)
                INST("srawi", b11_5, S, b16_5)

        case 826: // 413*3+0
            if (b31_1)
                INST("sradiRC", b11_5, S, b16_5)
                INST("sradi", b11_5, S, b16_5)

        case 827: // 413*3+1
            if (b31_1)
                INST("sradiRC", b11_5, S, b16_5)
                INST("sradi", b11_5, S, b16_5)

                // cache operations
        case 86:
            INST("dcbf", b11_5, b16_5);
        case 54:
            INST("dcbst", b11_5, b16_5);
        case 278:
            INST("dcbt", b11_5, b16_5);
        case 246:
            INST("dcbtst", b11_5, b16_5);
        case 1014:
            INST("dcbz", b11_5, b16_5);

            // lwarx/stwcx
        case 20:
            INST("lwarx", S, b11_5, b16_5)
        case 84:
            INST("ldarx", S, b11_5, b16_5)
        case 150:
            INST("stwcxRC", S, b11_5, b16_5)
        case 214:
            INST("stdcxRC", S, b11_5, b16_5)

                // mtspr, mfspr (move to/from system register)
        case 467:
            INST("mtspr", b11_10, b6_5)
        case 339:
            INST("mfspr", b6_5, b11_10)

                // MSR (Machine State Register)
        case 83:
            INST("mfmsr", b6_5)
        case 178:
            INST("mtmsrd", b6_5)

                // mtcrf, mtocrf
        case 144:
            if (b11_1 == 0)
                INST("mtcrf", b12_8, b6_5);
            INST("mtocrf", b12_8, b6_5);

            // mfcr, mfocrf
        case 19:
            if (b11_1 == 0)
                INST("mfcr", b6_5);
            INST("mfocrf", b12_8, b6_5);
        }

        uint32_t ext21_11 = ibf->GetAt(21, 11);
        switch (ext21_11) {
            // VMX LOADS
        case 12:
            INST("lvsl", b6_5, b11_5, b16_5)
        case 1038:
            INST("lvlx", S, b11_5, b16_5)
        case 1550:
            INST("lvlxl", S, b11_5, b16_5)
        case 1102:
            INST("lvrx", S, b11_5, b16_5)
        case 1614:
            INST("lvrxl", S, b11_5, b16_5)
        case 206:
            INST("lvx", S, b11_5, b16_5)
        case 718:
            INST("lvxl", S, b11_5, b16_5)
        case 76:
            INST("lvsr", S, b11_5, b16_5)

                // STORE

        case 270:
            INST("stvebx", b6_5, b11_5, b16_5)
        case 334:
            INST("stvehx", b6_5, b11_5, b16_5)
        case 398:
            INST("stvewx", b6_5, b11_5, b16_5)
        case 1294:
            INST("stvlx", b6_5, b11_5, b16_5)
        case 1806:
            INST("stvlxl", b6_5, b11_5, b16_5)
        case 1358:
            INST("stvrx", b6_5, b11_5, b16_5)
        case 1870:
            INST("stvrxl", b6_5, b11_5, b16_5)
        case 462:
            INST("stvx", b6_5, b11_5, b16_5)
        case 974:
            INST("stvxl", b6_5, b11_5, b16_5)
        }

        // math extended
        switch (ext22_9) {
        case 10:
            if (!b21_1 && !b31_1)
                INST("addc", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("addcRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("addcOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("addcOERC", S, b11_5, b16_5);

            // neg, neg., nego, nego.
        case 104:
            if (!b21_1 && !b31_1)
                INST("neg", S, b11_5);
            if (!b21_1 && b31_1)
                INST("negRC", S, b11_5);
            if (b21_1 && !b31_1)
                INST("negOE", S, b11_5);
            if (b21_1 && b31_1)
                INST("negOERC", S, b11_5);

            // subfe, subfe., subfeo, subfeo.
        case 136:
            if (!b21_1 && !b31_1)
                INST("subfe", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("subfeRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("subfeOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("subfeOERC", S, b11_5, b16_5);

        case 138:
            if (!b21_1 && !b31_1)
                INST("adde", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("addeRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("addeOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("addeOERC", S, b11_5, b16_5);

        case 200:
            if (!b21_1 && !b31_1)
                INST("subfze", S, b11_5);
            if (!b21_1 && b31_1)
                INST("subfzeRC", S, b11_5);
            if (b21_1 && !b31_1)
                INST("subfzeOE", S, b11_5);
            if (b21_1 && b31_1)
                INST("subfzeOERC", S, b11_5);

            // mullw, mullw., mullwo, mullwo.
        case 235:
            if (!b21_1 && !b31_1)
                INST("mullw", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("mullwRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("mullwOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("mullwOERC", S, b11_5, b16_5);

        case 233:
            if (!b21_1 && !b31_1)
                INST("mulld", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("mulldRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("mulldOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("mulldOERC", S, b11_5, b16_5);

            // add rD,rA,rB (also RC OE)
        case 266:
            if (!b21_1 && !b31_1)
                INST("add", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("addRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("addOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("addOERC", S, b11_5, b16_5);

            // addze, addze., addzeo, addzeo.
        case 202:
            if (!b21_1 && !b31_1)
                INST("addze", S, b11_5);
            if (!b21_1 && b31_1)
                INST("addzeRC", S, b11_5);
            if (b21_1 && !b31_1)
                INST("addzeOE", S, b11_5);
            if (b21_1 && b31_1)
                INST("addzeOERC", S, b11_5);

            // subfc, subfc., subfco, subfco.
        case 8:
            if (!b21_1 && !b31_1)
                INST("subfc", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("subfcRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("subfcOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("subfcOERC", S, b11_5, b16_5);

            // subf rD,rA,rB (also RC OE)
        case 40:
            if (!b21_1 && !b31_1)
                INST("subf", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("subfRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("subfOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("subfOERC", S, b11_5, b16_5);

        case 234:
            if (!b21_1 && !b31_1)
                INST("addme", S, b11_5);
            if (!b21_1 && b31_1)
                INST("addmeRC", S, b11_5);
            if (b21_1 && !b31_1)
                INST("addmeOE", S, b11_5);
            if (b21_1 && b31_1)
                INST("addmeOERC", S, b11_5);

        case 489:
            if (!b21_1 && !b31_1)
                INST("divd", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("divdRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("divdOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("divdOERC", S, b11_5, b16_5);

            // divw, divw., divwo, divwo.
        case 491:
            if (!b21_1 && !b31_1)
                INST("divw", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("divwRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("divwOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("divwOERC", S, b11_5, b16_5);

            // divdu, divdu., divduo, divduo.
        case 457:
            if (!b21_1 && !b31_1)
                INST("divdu", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("divduRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("divduOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("divduOERC", S, b11_5, b16_5);

            // divwu, divwu., divwuo, divwuo.
        case 459:
            if (!b21_1 && !b31_1)
                INST("divwu", S, b11_5, b16_5);
            if (!b21_1 && b31_1)
                INST("divwuRC", S, b11_5, b16_5);
            if (b21_1 && !b31_1)
                INST("divwuOE", S, b11_5, b16_5);
            if (b21_1 && b31_1)
                INST("divwuOERC", S, b11_5, b16_5);
        }

        printf("EXTENDED OPS NOT IMPLEMENTED EXTOC21_10: %i EXTOC21_11: %i EXTOC22_9: %i OP: %i\n",
            ext21_10,
            ext21_11,
            ext22_9,
            opcode);
        return 0;
    }

    case 32:
        INST("lwz", S, b16_16, b11_5) // lwz rS,d(rA)
    case 33:
        INST("lwzu", S, b16_16, b11_5) // lwzu rS,d(rA)
    case 34:
        INST("lbz", S, b16_16, b11_5); // lbz rS,d(rA)
    case 35:
        INST("lbzu", S, b16_16, b11_5); // lbzu rS,d(rA)
    case 36:
        INST("stw", S, b16_16, b11_5) // stw rS,d(rA)
    case 37:
        INST("stwu", S, b16_16, b11_5) // stwu rS,d(rA)
    case 38:
        INST("stb", S, b16_16, b11_5) // stb rS,d(rA)
    case 39:
        INST("stbu", S, b16_16, b11_5) // stbu rS,d(rA)
    case 40:
        INST("lhz", S, b16_16, b11_5) // lhz rS,d(rA)
    case 41:
        INST("lhzu", S, b16_16, b11_5) // lhzu rS,d(rA)
    case 42:
        INST("lha", S, b16_16, b11_5) // lha rS,d(rA)
    case 43:
        INST("lhau", S, b16_16, b11_5) // lhau rS,d(rA)
    case 44:
        INST("sth", S, b16_16, b11_5) // sth rS,d(rA)
    case 45:
        INST("sthu", S, b16_16, b11_5) // sthu rS,d(rA)

    case 48:
        INST("lfs", S, b16_16, b11_5) // lfs frD,d(rA)
    case 49:
        INST("lfsu", S, b16_16, b11_5); // lfsu frD, d(rA)
    case 50:
        INST("lfd", S, b16_16, b11_5) // lfd frD, d(rA)
    case 51:
        INST("lfdu", S, b16_16, b11_5); // lfdu frD, d(rA)
    case 52:
        INST("stfs", S, b16_16, b11_5) // stfs frS,d(rA)
    case 53:
        INST("stfsu", S, b16_16, b11_5); // stfsu frS, d(rA)
    case 54:
        INST("stfd", S, b16_16, b11_5) // stfd frS, d(rA)
    case 55:
        INST("stfdu", S, b16_16, b11_5); // stfdu frS, d(rA)

        // offset is *4
    case 58:
        if (aa && !b31_1)
            INST("lwa", S, b16_14, b11_5) // lwa rD,ds(rA)
            if (!aa && !b31_1)
                INST("ld", S, b16_14, b11_5) // ld rD,ds(rA)
                if (!aa && b31_1)
                    INST("ldu", S, b16_14, b11_5) // ldu rD,ds(rA)

    case 62:
        if (!aa && !b31_1)
            INST("std", S, b16_14, b11_5) // std rS,ds(rA)
            if (!aa && b31_1)
                INST("stdu", S, b16_14, b11_5) // stdu rS,ds(rA) if lk == 1

                // single floating math wowoy
    case 59: {
            switch (ext26_5) {
            case 20:
                if (b31_1)
                    INST("fsubsRC", S, b11_5, b16_5)
                    INST("fsubs", S, b11_5, b16_5)
            case 21:
                if (b31_1)
                    INST("faddsRC", S, b11_5, b16_5)
                    INST("fadds", S, b11_5, b16_5)
                    // fmuls frD,frA,frC (also RC)
            case 25:
                if (b31_1)
                    INST("fmulsRC", S, b11_5, b21_5)
                    INST("fmuls", S, b11_5, b21_5)
            case 18:
                if (b31_1)
                    INST("fdivsRC", S, b11_5, b16_5)
                    INST("fdivs", S, b11_5, b16_5)
                    // fmadd, fmsub, fnmadd, fnmsub
            case 29:
                if (b31_1)
                    INST("fmaddsRC", S, b11_5, b21_5, b16_5)
                    INST("fmadds", S, b11_5, b21_5, b16_5)

            case 28:
                if (b31_1)
                    INST("fmsubsRC", S, b11_5, b21_5, b16_5)
                    INST("fmsubs", S, b11_5, b21_5, b16_5)

            case 31:
                if (b31_1)
                    INST("fnmaddsRC", S, b11_5, b21_5, b16_5)
                    INST("fnmadds", S, b11_5, b21_5, b16_5)

            case 30:
                if (b31_1)
                    INST("fnmsubsRC", S, b11_5, b21_5, b16_5)
                    INST("fnmsubs", S, b11_5, b21_5, b16_5)

                    // fsqrts, fres
            case 22:
                if (b31_1)
                    INST("fsqrtsRC", S, b16_5)
                    INST("fsqrts", S, b16_5)

            case 24:
                if (b31_1)
                    INST("fresRC", S, b16_5)
                    INST("fres", S, b16_5)
            }

            printf("EXTENDED SINGLE FLOATING OPS NOT IMPLEMENTED EXTOC26_5: %i OP: %i\n", ext26_5, opcode);
            return 0;
        }

        // double floating math wowoy
    case 63: {
        switch (ext21_10) {
        case 0:
            INST("fcmpu", b6_3, b11_5, b16_5)
        case 32:
            INST("fcmpo", b6_3, b11_5, b16_5)

        case 814:
            if (b31_1)
                INST("fctidRC", S, b16_5)
                INST("fctid", S, b16_5)

        case 14:
            if (b31_1)
                INST("fctiwRC", S, b16_5)
                INST("fctiw", S, b16_5)

        case 15:
            if (b31_1)
                INST("fctiwzRC", S, b16_5)
                INST("fctiwz", S, b16_5)

                // frsp frD,frB (also RC)
        case 12:
            if (b31_1)
                INST("frspRC", S, b16_5)
                INST("frsp", S, b16_5)

                // fneg frD,frB (also RC)
        case 40:
            if (b31_1)
                INST("fnegRC", S, b16_5)
                INST("fneg", S, b16_5)

                // fmr frD, frB (also RC)
        case 72:
            if (b31_1)
                INST("fmrRC", S, b16_5)
                INST("fmr", S, b16_5)

        case 264:
            if (b31_1)
                INST("fabsRC", S, b16_5)
                INST("fabs", S, b16_5)

        case 136:
            if (b31_1)
                INST("fnabsRC", S, b16_5)
                INST("fnabs", S, b16_5)

        case 815:
            if (b31_1)
                INST("fctidzRC", S, b16_5)
                INST("fctidz", S, b16_5)

                // fcfid frD,frB (also RC)
        case 846:
            if (b31_1)
                INST("fcfidRC", S, b16_5)
                INST("fcfid", S, b16_5)

        case 583:
            if (b31_1)
                INST("mffsRC", b6_5)
                INST("mffs", b6_5)

        case 711:
            if (b31_1)
                INST("mtfsfRC", b7_8, b16_5)
                INST("mtfsf", b7_8, b16_5)
        }

        switch (ext26_5) {
            // fadd, fsub, fmul, fdiv
        case 21:
            if (b31_1)
                INST("faddRC", S, b11_5, b16_5)
                INST("fadd", S, b11_5, b16_5)

        case 20:
            if (b31_1)
                INST("fsubRC", S, b11_5, b16_5)
                INST("fsub", S, b11_5, b16_5)

        case 25: // BEWARE, the second arg is at bit 21
            if (b31_1)
                INST("fmulRC", S, b11_5, b16_5)
                INST("fmul", S, b11_5, b16_5)

        case 18:
            if (b31_1)
                INST("fdivRC", S, b11_5, b16_5)
                INST("fdiv", S, b11_5, b16_5)

                // fmadd, fmsub, fnmadd, fnmsub
        case 29:
            if (b31_1)
                INST("fmaddRC", S, b11_5, b21_5, b16_5)
                INST("fmadd", S, b11_5, b21_5, b16_5)

        case 28:
            if (b31_1)
                INST("fmsubRC", S, b11_5, b21_5, b16_5)
                INST("fmsub", S, b11_5, b21_5, b16_5)

        case 31:
            if (b31_1)
                INST("fnmaddRC", S, b11_5, b21_5, b16_5)
                INST("fnmadd", S, b11_5, b21_5, b16_5)

        case 30:
            if (b31_1)
                INST("fnmsubRC", S, b11_5, b21_5, b16_5)
                INST("fnmsub", S, b11_5, b21_5, b16_5)

                // fsel frD,frA,frC,frB
        case 23:
            if (b31_1)
                INST("fselRC", S, b11_5, b21_5, b16_5)
                INST("fsel", S, b11_5, b21_5, b16_5)

                // fsqrt and some other optional instructions
        case 22:
            if (b31_1)
                INST("fsqrtRC", S, b16_5)
                INST("fsqrt", S, b16_5)

        case 24:
            if (b31_1)
                INST("freRC", S, b16_5)
                INST("fre", S, b16_5)

        case 26:
            if (b31_1)
                INST("frsqrteRC", S, b16_5)
                INST("frsqrte", S, b16_5)
        }

        printf("EXTENDED DOUBLE FLOATING OPS NOT IMPLEMENTED EXTOC: %i OP: %i\n", ext21_10, opcode);
        return 0;
    }
    }

    printf("UNABLE TO DECODE INSTRUCTION: opcode: %i\n", opcode);
    DebugBreak();
    printf("Unknown instruction %08X\n", instr);

    return 0;
}
