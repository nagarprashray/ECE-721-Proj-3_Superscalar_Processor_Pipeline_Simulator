#ifndef RENAMER_H
#define RENAMER_H

#include <inttypes.h>
#include <vector>
#include <cassert>

class renamer {
private:
    struct rmt {
        int phy_reg_mapping;
    };

    struct amt {
        int committed_phy_reg_mapping;
    };

    struct freeList {
        int phy_reg_num;
    };

    struct activeList {
        int dest_flag;
        int logical_reg_num;
        int phy_reg_num;
        int complete_bit;
        int execp_bit;
        bool load_viol_bit, br_mispred_bit, val_mispred_bit;
        int load_flg, str_flg, br_flg, amo_flg, csr_flg;
        uint64_t pc;
    };

    struct prf {
        uint64_t value;
    };

    struct prf_rdy_bit_ary {
        int rdy;
    };

    struct br_checkpoints {
        std::vector<rmt> SMT;
        int head, head_ph;
        uint64_t gbm_check;
    };

    std::vector<rmt> RMT;
    std::vector<amt> AMT;
    std::vector<freeList> FL;
    std::vector<activeList> AL;
    std::vector<prf> PRF;
    std::vector<prf_rdy_bit_ary> RDY_BIT;
    std::vector<br_checkpoints> BR_CHK;

    int fl_head, fl_tail, fl_h_phase, fl_t_phase;
    int al_head, al_tail, al_h_phase, al_t_phase;
    int free_list_entries, active_list_empty_entries;

    uint64_t GBM;

    bool is_FL_empty();
    bool is_FL_full();
    bool is_AL_empty();
    bool is_AL_full();
    int avail_FL_entries();
    int avail_AL_entries();
    uint64_t set_GBM_bit();
    void clear_GBM_bit(uint64_t branch_id);
    uint64_t clear_bit(uint64_t branch_id, uint64_t chk_gbm);

public:
    renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches, uint64_t n_active);
    ~renamer();

    bool stall_reg(uint64_t bundle_dst);
    bool stall_branch(uint64_t bundle_branch);
    uint64_t get_branch_mask();
    uint64_t rename_rsrc(uint64_t log_reg);
    uint64_t rename_rdst(uint64_t log_reg);
    uint64_t checkpoint();

    bool stall_dispatch(uint64_t bundle_inst);
    uint64_t dispatch_inst(bool dest_valid, uint64_t log_reg, uint64_t phys_reg, bool load, bool store, bool branch, bool amo, bool csr, uint64_t PC);
    bool is_ready(uint64_t phys_reg);
    void clear_ready(uint64_t phys_reg);

    uint64_t read(uint64_t phys_reg);
    void set_ready(uint64_t phys_reg);
    void write(uint64_t phys_reg, uint64_t value);
    void set_complete(uint64_t AL_index);

    void resolve(uint64_t AL_index, uint64_t branch_ID, bool correct);

    bool precommit(bool &completed, bool &exception, bool &load_viol, bool &br_misp, bool &val_misp, bool &load, bool &store, bool &branch, bool &amo, bool &csr, uint64_t &PC);
    void commit();

    void squash();

    void set_exception(uint64_t AL_index);
    void set_load_violation(uint64_t AL_index);
    void set_branch_misprediction(uint64_t AL_index);
    void set_value_misprediction(uint64_t AL_index);
    bool get_exception(uint64_t AL_index);
};

#endif // RENAMER_H