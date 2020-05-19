// unit tests for angle styles intended for molecular systems

#include "yaml_reader.h"
#include "yaml_writer.h"
#include "error_stats.h"
#include "test_config.h"
#include "test_config_reader.h"
#include "test_main.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "lammps.h"
#include "atom.h"
#include "modify.h"
#include "compute.h"
#include "force.h"
#include "angle.h"
#include "info.h"
#include "input.h"
#include "universe.h"

#include <mpi.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <ctime>

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

using ::testing::StartsWith;
using ::testing::HasSubstr;

void cleanup_lammps(LAMMPS_NS::LAMMPS *lmp, const TestConfig &cfg)
{
    std::string name;

    name = cfg.basename + ".restart";
    remove(name.c_str());
    name = cfg.basename + ".data";
    remove(name.c_str());
    name = cfg.basename + "-coeffs.in";
    remove(name.c_str());
    delete lmp;
}

LAMMPS_NS::LAMMPS *init_lammps(int argc, char **argv,
                               const TestConfig &cfg,
                               const bool newton=true)
{
    LAMMPS_NS::LAMMPS *lmp;

    lmp = new LAMMPS_NS::LAMMPS(argc, argv, MPI_COMM_WORLD);

    // check if prerequisite styles are available
    LAMMPS_NS::Info *info = new LAMMPS_NS::Info(lmp);
    int nfail = 0;
    for (auto prerequisite : cfg.prerequisites) {
        std::string style = prerequisite.second;

        // this is a test for angle styles, so if the suffixed
        // version is not available, there is no reason to test.
        if (prerequisite.first == "angle") {
            if (lmp->suffix_enable) {
                style += "/";
                style += lmp->suffix;
            }
        }

        if (!info->has_style(prerequisite.first,style)) ++nfail;
    }
    if (nfail > 0) {
        delete info;
        cleanup_lammps(lmp,cfg);
        return NULL;
    }

    if (newton) {
        lmp->input->one("variable newton_bond index on");
    } else {
        lmp->input->one("variable newton_bond index off");
    }

#define STRINGIFY(val) XSTR(val)
#define XSTR(val) #val
    std::string set_input_dir = "variable input_dir index ";
    set_input_dir += STRINGIFY(TEST_INPUT_FOLDER);
    lmp->input->one(set_input_dir.c_str());
    for (auto pre_command : cfg.pre_commands)
        lmp->input->one(pre_command.c_str());

    std::string input_file = STRINGIFY(TEST_INPUT_FOLDER);
    input_file += "/";
    input_file += cfg.input_file;
    lmp->input->file(input_file.c_str());
#undef STRINGIFY
#undef XSTR

    std::string cmd("angle_style ");
    cmd += cfg.angle_style;
    lmp->input->one(cmd.c_str());
    for (auto angle_coeff : cfg.angle_coeff) {
        cmd = "angle_coeff " + angle_coeff;
        lmp->input->one(cmd.c_str());
    }
    for (auto post_command : cfg.post_commands)
        lmp->input->one(post_command.c_str());
    lmp->input->one("run 0 post no");
    cmd = "write_restart " + cfg.basename + ".restart";
    lmp->input->one(cmd.c_str());
    cmd = "write_data " + cfg.basename + ".data";
    lmp->input->one(cmd.c_str());
    cmd = "write_coeff " + cfg.basename + "-coeffs.in";
    lmp->input->one(cmd.c_str());

    return lmp;
}

void run_lammps(LAMMPS_NS::LAMMPS *lmp)
{
    lmp->input->one("fix 1 all nve");
    lmp->input->one("compute pe all pe/atom");
    lmp->input->one("compute sum all reduce sum c_pe");
    lmp->input->one("thermo_style custom step temp pe press c_sum");
    lmp->input->one("thermo 2");
    lmp->input->one("run 4 post no");
}

void restart_lammps(LAMMPS_NS::LAMMPS *lmp, const TestConfig &cfg)
{
    lmp->input->one("clear");
    std::string cmd("read_restart ");
    cmd += cfg.basename + ".restart";
    lmp->input->one(cmd.c_str());

    if (!lmp->force->angle) {
        cmd = "angle_style " + cfg.angle_style;
        lmp->input->one(cmd.c_str());
    }
    if ((cfg.angle_style.substr(0,6) == "hybrid")
        || !lmp->force->angle->writedata) {
        for (auto angle_coeff : cfg.angle_coeff) {
            cmd = "angle_coeff " + angle_coeff;
            lmp->input->one(cmd.c_str());
        }
    }
    for (auto post_command : cfg.post_commands)
        lmp->input->one(post_command.c_str());
    lmp->input->one("run 0 post no");
}

void data_lammps(LAMMPS_NS::LAMMPS *lmp, const TestConfig &cfg)
{
    lmp->input->one("clear");
    lmp->input->one("variable angle_style delete");
    lmp->input->one("variable data_file  delete");
    lmp->input->one("variable newton_bond delete");
    lmp->input->one("variable newton_bond index on");

    for (auto pre_command : cfg.pre_commands)
        lmp->input->one(pre_command.c_str());

    std::string cmd("variable angle_style index '");
    cmd += cfg.angle_style + "'";
    lmp->input->one(cmd.c_str());

    cmd = "variable data_file index ";
    cmd += cfg.basename + ".data";
    lmp->input->one(cmd.c_str());

#define STRINGIFY(val) XSTR(val)
#define XSTR(val) #val
    std::string input_file = STRINGIFY(TEST_INPUT_FOLDER);
    input_file += "/";
    input_file += cfg.input_file;
    lmp->input->file(input_file.c_str());
#undef STRINGIFY
#undef XSTR

    for (auto angle_coeff : cfg.angle_coeff) {
        cmd = "angle_coeff " + angle_coeff;
        lmp->input->one(cmd.c_str());
    }
    for (auto post_command : cfg.post_commands)
        lmp->input->one(post_command.c_str());
    lmp->input->one("run 0 post no");
}

class AngleConfigReader : public TestConfigReader 
{
public:
    AngleConfigReader(TestConfig &config) : TestConfigReader(config) {        
        consumers["angle_style"]     = &TestConfigReader::angle_style;
        consumers["angle_coeff"]     = &TestConfigReader::angle_coeff;
        consumers["init_energy"]    = &TestConfigReader::init_energy;
        consumers["run_energy"]     = &TestConfigReader::run_energy;
    }
};

// read/parse yaml file

bool read_yaml_file(const char *infile, TestConfig &config)
{
    auto reader = AngleConfigReader(config);
    if (reader.parse_file(infile))
        return false;
    
    config.basename = reader.get_basename();
    return true;
}

// re-generate yaml file with current settings.

void generate_yaml_file(const char *outfile, const TestConfig &config)
{
    // initialize system geometry
    const char *args[] = {"AngleStyle", "-log", "none", "-echo", "screen", "-nocite" };
    char **argv = (char **)args;
    int argc = sizeof(args)/sizeof(char *);
    LAMMPS_NS::LAMMPS *lmp = init_lammps(argc,argv,config);
    if (!lmp) {
        std::cerr << "One or more prerequisite styles are not available "
            "in this LAMMPS configuration:\n";
        for (auto prerequisite : config.prerequisites) {
            std::cerr << prerequisite.first << "_style "
                      << prerequisite.second << "\n";
        }
        return;
    }

    const int natoms = lmp->atom->natoms;
    const int bufsize = 256;
    char buf[bufsize];
    std::string block("");

    YamlWriter writer(outfile);

    // lammps_version
    writer.emit("lammps_version", lmp->universe->version);

    // date_generated
    std::time_t now = time(NULL);
    block = ctime(&now);
    block = block.substr(0,block.find("\n")-1);
    writer.emit("date_generated", block);

    // epsilon
    writer.emit("epsilon", config.epsilon);

    // prerequisites
    block.clear();
    for (auto prerequisite :  config.prerequisites) {
        block += prerequisite.first + " " + prerequisite.second + "\n";
    }
    writer.emit_block("prerequisites", block);

    // pre_commands
    block.clear();
    for (auto command :  config.pre_commands) {
        block += command + "\n";
    }
    writer.emit_block("pre_commands", block);

    // post_commands
    block.clear();
    for (auto command : config.post_commands) {
        block += command + "\n";
    }
    writer.emit_block("post_commands", block);

    // input_file
    writer.emit("input_file", config.input_file);

    // angle_style
    writer.emit("angle_style", config.angle_style);

    // angle_coeff
    block.clear();
    for (auto angle_coeff : config.angle_coeff) {
        block += angle_coeff + "\n";
    }
    writer.emit_block("angle_coeff", block);

    // extract
    block.clear();
    std::stringstream outstr;
    for (auto data : config.extract) {
        outstr << data.first << " " << data.second << std::endl;
    }
    writer.emit_block("extract", outstr.str());

    // natoms
    writer.emit("natoms", natoms);

    // init_energy
    writer.emit("init_energy", lmp->force->angle->energy);

    // init_stress
    double *stress = lmp->force->angle->virial;
    snprintf(buf,bufsize,"% 23.16e % 23.16e % 23.16e % 23.16e % 23.16e % 23.16e",
             stress[0],stress[1],stress[2],stress[3],stress[4],stress[5]);
    writer.emit_block("init_stress", buf);

    // init_forces
    block.clear();
    double **f = lmp->atom->f;
    LAMMPS_NS::tagint *tag = lmp->atom->tag;
    for (int i=0; i < natoms; ++i) {
        snprintf(buf,bufsize,"% 3d % 23.16e % 23.16e % 23.16e\n",
                 (int)tag[i], f[i][0], f[i][1], f[i][2]);
        block += buf;
    }
    writer.emit_block("init_forces", block);

    // do a few steps of MD
    run_lammps(lmp);

    // run_energy
    writer.emit("run_energy", lmp->force->angle->energy);

    // run_stress
    stress = lmp->force->angle->virial;
    snprintf(buf,bufsize,"% 23.16e % 23.16e % 23.16e % 23.16e % 23.16e % 23.16e",
             stress[0],stress[1],stress[2],stress[3],stress[4],stress[5]);
    writer.emit_block("run_stress", buf);

    block.clear();
    f = lmp->atom->f;
    tag = lmp->atom->tag;
    for (int i=0; i < natoms; ++i) {
        snprintf(buf,bufsize,"% 3d % 23.16e % 23.16e % 23.16e\n",
                 (int)tag[i], f[i][0], f[i][1], f[i][2]);
        block += buf;
    }
    writer.emit_block("run_forces", block);

    cleanup_lammps(lmp,config);
    return;
}

TEST(AngleStyle, plain) {
    const char *args[] = {"AngleStyle", "-log", "none", "-echo", "screen", "-nocite" };
    char **argv = (char **)args;
    int argc = sizeof(args)/sizeof(char *);

    ::testing::internal::CaptureStdout();
    LAMMPS_NS::LAMMPS *lmp = init_lammps(argc,argv,test_config,true);
    std::string output = ::testing::internal::GetCapturedStdout();

    if (!lmp) {
        std::cerr << "One or more prerequisite styles are not available "
            "in this LAMMPS configuration:\n";
        for (auto prerequisite : test_config.prerequisites) {
            std::cerr << prerequisite.first << "_style "
                      << prerequisite.second << "\n";
        }
        GTEST_SKIP();
    }

    EXPECT_THAT(output, StartsWith("LAMMPS ("));
    EXPECT_THAT(output, HasSubstr("Loop time"));

    // abort if running in parallel and not all atoms are local
    const int nlocal = lmp->atom->nlocal;
    ASSERT_EQ(lmp->atom->natoms,nlocal);

    double epsilon = test_config.epsilon;
    double **f=lmp->atom->f;
    LAMMPS_NS::tagint *tag=lmp->atom->tag;
    ErrorStats stats;
    stats.reset();
    const std::vector<coord_t> &f_ref = test_config.init_forces;
    ASSERT_EQ(nlocal+1,f_ref.size());
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_ref[tag[i]].x, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_ref[tag[i]].y, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_ref[tag[i]].z, epsilon);
    }
    if (print_stats)
        std::cerr << "init_forces stats, newton on: " << stats << std::endl;

    LAMMPS_NS::Angle *angle = lmp->force->angle;
    double *stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.init_stress.xx, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.init_stress.yy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.init_stress.zz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.init_stress.xy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.init_stress.xz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.init_stress.yz, epsilon);
    if (print_stats)
        std::cerr << "init_stress stats, newton on: " << stats << std::endl;

    stats.reset();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.init_energy, epsilon);
    if (print_stats)
        std::cerr << "init_energy stats, newton on: " << stats << std::endl;

    ::testing::internal::CaptureStdout();
    run_lammps(lmp);
    ::testing::internal::GetCapturedStdout();

    f = lmp->atom->f;
    stress = angle->virial;
    const std::vector<coord_t> &f_run = test_config.run_forces;
    ASSERT_EQ(nlocal+1,f_run.size());
    stats.reset();
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_run[tag[i]].x, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_run[tag[i]].y, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_run[tag[i]].z, 10*epsilon);
    }
    if (print_stats)
        std::cerr << "run_forces  stats, newton on: " << stats << std::endl;

    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.run_stress.xx, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.run_stress.yy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.run_stress.zz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.run_stress.xy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.run_stress.xz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.run_stress.yz, epsilon);
    if (print_stats)
        std::cerr << "run_stress  stats, newton on: " << stats << std::endl;

    stats.reset();
    int id = lmp->modify->find_compute("sum");
    double energy = lmp->modify->compute[id]->compute_scalar();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.run_energy, epsilon);
    EXPECT_FP_LE_WITH_EPS(angle->energy, energy, epsilon);
    if (print_stats)
        std::cerr << "run_energy  stats, newton on: " << stats << std::endl;

    ::testing::internal::CaptureStdout();
    cleanup_lammps(lmp,test_config);
    lmp = init_lammps(argc,argv,test_config,false);
    output = ::testing::internal::GetCapturedStdout();

    f=lmp->atom->f;
    tag=lmp->atom->tag;
    stats.reset();
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_ref[tag[i]].x, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_ref[tag[i]].y, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_ref[tag[i]].z, epsilon);
    }
    if (print_stats)
        std::cerr << "init_forces stats, newton off:" << stats << std::endl;

    angle = lmp->force->angle;
    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.init_stress.xx, 2*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.init_stress.yy, 2*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.init_stress.zz, 2*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.init_stress.xy, 2*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.init_stress.xz, 2*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.init_stress.yz, 2*epsilon);
    if (print_stats)
        std::cerr << "init_stress stats, newton off:" << stats << std::endl;

    stats.reset();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.init_energy, epsilon);
    if (print_stats)
        std::cerr << "init_energy stats, newton off:" << stats << std::endl;

    ::testing::internal::CaptureStdout();
    run_lammps(lmp);
    ::testing::internal::GetCapturedStdout();

    f = lmp->atom->f;
    stress = angle->virial;
    stats.reset();
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_run[tag[i]].x, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_run[tag[i]].y, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_run[tag[i]].z, 10*epsilon);
    }
    if (print_stats)
        std::cerr << "run_forces  stats, newton off:" << stats << std::endl;

    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.run_stress.xx, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.run_stress.yy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.run_stress.zz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.run_stress.xy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.run_stress.xz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.run_stress.yz, epsilon);
    if (print_stats)
        std::cerr << "run_stress  stats, newton off:" << stats << std::endl;

    stats.reset();
    id = lmp->modify->find_compute("sum");
    energy = lmp->modify->compute[id]->compute_scalar();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.run_energy, epsilon);
    EXPECT_FP_LE_WITH_EPS(angle->energy, energy, epsilon);
    if (print_stats)
        std::cerr << "run_energy  stats, newton off:" << stats << std::endl;

    ::testing::internal::CaptureStdout();
    restart_lammps(lmp, test_config);
    ::testing::internal::GetCapturedStdout();

    f=lmp->atom->f;
    tag=lmp->atom->tag;
    stats.reset();
    ASSERT_EQ(nlocal+1,f_ref.size());
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_ref[tag[i]].x, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_ref[tag[i]].y, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_ref[tag[i]].z, epsilon);
    }
    if (print_stats)
        std::cerr << "restart_forces stats:" << stats << std::endl;

    angle = lmp->force->angle;
    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.init_stress.xx, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.init_stress.yy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.init_stress.zz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.init_stress.xy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.init_stress.xz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.init_stress.yz, epsilon);
    if (print_stats)
        std::cerr << "restart_stress stats:" << stats << std::endl;

    stats.reset();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.init_energy, epsilon);
    if (print_stats)
        std::cerr << "restart_energy stats:" << stats << std::endl;

    ::testing::internal::CaptureStdout();
    data_lammps(lmp, test_config);
    ::testing::internal::GetCapturedStdout();

    f=lmp->atom->f;
    tag=lmp->atom->tag;
    stats.reset();
    ASSERT_EQ(nlocal+1,f_ref.size());
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_ref[tag[i]].x, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_ref[tag[i]].y, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_ref[tag[i]].z, epsilon);
    }
    if (print_stats)
        std::cerr << "data_forces stats:" << stats << std::endl;

    angle = lmp->force->angle;
    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.init_stress.xx, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.init_stress.yy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.init_stress.zz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.init_stress.xy, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.init_stress.xz, epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.init_stress.yz, epsilon);
    if (print_stats)
        std::cerr << "data_stress stats:" << stats << std::endl;

    stats.reset();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.init_energy, epsilon);
    if (print_stats)
        std::cerr << "data_energy stats:" << stats << std::endl;

    ::testing::internal::CaptureStdout();
    cleanup_lammps(lmp,test_config);
    ::testing::internal::GetCapturedStdout();
};

TEST(AngleStyle, omp) {
    if (!LAMMPS_NS::LAMMPS::is_installed_pkg("USER-OMP")) GTEST_SKIP();
    const char *args[] = {"AngleStyle", "-log", "none", "-echo", "screen",
                          "-nocite", "-pk", "omp", "4", "-sf", "omp"};
    char **argv = (char **)args;
    int argc = sizeof(args)/sizeof(char *);

    ::testing::internal::CaptureStdout();
    LAMMPS_NS::LAMMPS *lmp = init_lammps(argc,argv,test_config,true);
    std::string output = ::testing::internal::GetCapturedStdout();

    if (!lmp) {
        std::cerr << "One or more prerequisite styles with /omp suffix\n"
            "are not available in this LAMMPS configuration:\n";
        for (auto prerequisite : test_config.prerequisites) {
            std::cerr << prerequisite.first << "_style "
                      << prerequisite.second << "\n";
        }
        GTEST_SKIP();
    }

    EXPECT_THAT(output, StartsWith("LAMMPS ("));
    EXPECT_THAT(output, HasSubstr("Loop time"));

    // abort if running in parallel and not all atoms are local
    const int nlocal = lmp->atom->nlocal;
    ASSERT_EQ(lmp->atom->natoms,nlocal);

    // relax error a bit for USER-OMP package
    double epsilon = 5.0*test_config.epsilon;
    double **f=lmp->atom->f;
    LAMMPS_NS::tagint *tag=lmp->atom->tag;
    const std::vector<coord_t> &f_ref = test_config.init_forces;
    ErrorStats stats;
    stats.reset();
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_ref[tag[i]].x, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_ref[tag[i]].y, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_ref[tag[i]].z, epsilon);
    }
    if (print_stats)
        std::cerr << "init_forces stats, newton on: " << stats << std::endl;

    LAMMPS_NS::Angle *angle = lmp->force->angle;
    double *stress = angle->virial;

    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.init_stress.xx, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.init_stress.yy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.init_stress.zz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.init_stress.xy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.init_stress.xz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.init_stress.yz, 10*epsilon);
    if (print_stats)
        std::cerr << "init_stress stats, newton on: " << stats << std::endl;

    stats.reset();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.init_energy, epsilon);
    if (print_stats)
        std::cerr << "init_energy stats, newton on: " << stats << std::endl;

    ::testing::internal::CaptureStdout();
    run_lammps(lmp);
    ::testing::internal::GetCapturedStdout();

    f = lmp->atom->f;
    stress = angle->virial;
    const std::vector<coord_t> &f_run = test_config.run_forces;
    ASSERT_EQ(nlocal+1,f_run.size());
    stats.reset();
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_run[tag[i]].x, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_run[tag[i]].y, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_run[tag[i]].z, 10*epsilon);
    }
    if (print_stats)
        std::cerr << "run_forces  stats, newton on: " << stats << std::endl;

    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.run_stress.xx, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.run_stress.yy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.run_stress.zz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.run_stress.xy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.run_stress.xz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.run_stress.yz, 10*epsilon);
    if (print_stats)
        std::cerr << "run_stress  stats, newton on: " << stats << std::endl;

    stats.reset();
    int id = lmp->modify->find_compute("sum");
    double energy = lmp->modify->compute[id]->compute_scalar();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.run_energy, epsilon);
    // TODO: this is currently broken for USER-OMP with angle style hybrid
    // needs to be fixed in the main code somewhere. Not sure where, though.
    if (test_config.angle_style.substr(0,6) != "hybrid")
        EXPECT_FP_LE_WITH_EPS(angle->energy, energy, epsilon);
    if (print_stats)
        std::cerr << "run_energy  stats, newton on: " << stats << std::endl;

    ::testing::internal::CaptureStdout();
    cleanup_lammps(lmp,test_config);
    lmp = init_lammps(argc,argv,test_config,false);
    output = ::testing::internal::GetCapturedStdout();

    f=lmp->atom->f;
    tag=lmp->atom->tag;
    stats.reset();
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_ref[tag[i]].x, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_ref[tag[i]].y, epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_ref[tag[i]].z, epsilon);
    }
    if (print_stats)
        std::cerr << "init_forces stats, newton off:" << stats << std::endl;

    angle = lmp->force->angle;
    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.init_stress.xx, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.init_stress.yy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.init_stress.zz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.init_stress.xy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.init_stress.xz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.init_stress.yz, 10*epsilon);
    if (print_stats)
        std::cerr << "init_stress stats, newton off:" << stats << std::endl;

    stats.reset();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.init_energy, epsilon);
    if (print_stats)
        std::cerr << "init_energy stats, newton off:" << stats << std::endl;

    ::testing::internal::CaptureStdout();
    run_lammps(lmp);
    ::testing::internal::GetCapturedStdout();

    f = lmp->atom->f;
    stats.reset();
    for (int i=0; i < nlocal; ++i) {
        EXPECT_FP_LE_WITH_EPS(f[i][0], f_run[tag[i]].x, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][1], f_run[tag[i]].y, 10*epsilon);
        EXPECT_FP_LE_WITH_EPS(f[i][2], f_run[tag[i]].z, 10*epsilon);
    }
    if (print_stats)
        std::cerr << "run_forces  stats, newton off:" << stats << std::endl;

    stress = angle->virial;
    stats.reset();
    EXPECT_FP_LE_WITH_EPS(stress[0], test_config.run_stress.xx, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[1], test_config.run_stress.yy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[2], test_config.run_stress.zz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[3], test_config.run_stress.xy, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[4], test_config.run_stress.xz, 10*epsilon);
    EXPECT_FP_LE_WITH_EPS(stress[5], test_config.run_stress.yz, 10*epsilon);
    if (print_stats)
        std::cerr << "run_stress  stats, newton off:" << stats << std::endl;

    stats.reset();
    id = lmp->modify->find_compute("sum");
    energy = lmp->modify->compute[id]->compute_scalar();
    EXPECT_FP_LE_WITH_EPS(angle->energy, test_config.run_energy, epsilon);
    // TODO: this is currently broken for USER-OMP with angle style hybrid
    // needs to be fixed in the main code somewhere. Not sure where, though.
    if (test_config.angle_style.substr(0,6) != "hybrid")
        EXPECT_FP_LE_WITH_EPS(angle->energy, energy, epsilon);
    if (print_stats)
        std::cerr << "run_energy  stats, newton off:" << stats << std::endl;

    ::testing::internal::CaptureStdout();
    cleanup_lammps(lmp,test_config);
    ::testing::internal::GetCapturedStdout();
};
