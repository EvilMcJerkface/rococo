//#include "all.h"

#include "__dep__.h"
#include "multi_value.h"
#include "constants.h"
#include "config.h"
#include "multi_value.h"
#include "sharding.h"
#include "frame.h"
#include "sharding.h"
#include "rcc/dep_graph.h"
#include "rcc/graph_marshaler.h"
#include "piece.h"

// for tpca benchmark
#include "bench/tpca/piece.h"
#include "bench/tpca/chopper.h"

// tpcc benchmark
#include "bench/tpcc/piece.h"
#include "bench/tpcc/chopper.h"

// tpcc dist partition benchmark
#include "bench/tpcc_dist/piece.h"
#include "bench/tpcc_dist/chopper.h"

// tpcc real dist partition benchmark
#include "bench/tpcc_real_dist/piece.h"
#include "bench/tpcc_real_dist/chopper.h"

// rw benchmark
#include "bench/rw_benchmark/piece.h"
#include "bench/rw_benchmark/chopper.h"

// micro bench
#include "bench/micro/piece.h"
#include "bench/micro/chopper.h"



namespace rococo {
Config *Config::config_s = NULL;


Config * Config::GetConfig() {
  verify(config_s != nullptr);
  return config_s;
}

int Config::CreateConfig(int argc, char **argv) {
  if (config_s != NULL) return -1;

//  std::string filename = "./config/sample.yml";
  vector<string> config_paths;
  std::string proc_name = "localhost"; // default as "localhost"
  std::string logging_path = "./disk_log/";
  unsigned int sid = 0, cid = 0;
  char *end_ptr    = NULL;

  char *hostspath               = NULL;
  char *ctrl_hostname           = NULL;
  char *ctrl_key                = NULL;
  char *ctrl_init               = NULL /*, *ctrl_run = NULL*/;
  unsigned int ctrl_port        = 0;
  unsigned int ctrl_timeout     = 0;
  uint32_t duration         = 10;
  bool heart_beat               = false;
  single_server_t single_server = SS_DISABLED;
  int server_or_client          = -1;

  int c;
  optind = 1;
  string filename;
  while ((c = getopt(argc, argv, "bc:d:f:h:i:k:p:P:r:s:S:t:H:")) != -1) {
    switch (c) {
      case 'b': // heartbeat to controller
        heart_beat = true;
        break;
      case 'd': // duration
        duration = strtoul(optarg, &end_ptr, 10);

        if ((end_ptr == NULL) || (*end_ptr != '\0'))
          return -4;
        break;
      case 'P':
        proc_name = std::string(optarg);
        break;
      case 'f': // properties.xml
        filename = std::string(optarg);
        config_paths.push_back(filename);
        break;
      case 't':
        ctrl_timeout = strtoul(optarg, &end_ptr, 10);
        if ((end_ptr == NULL) || (*end_ptr != '\0')) return -4;
        break;
      case 'c': // client id
        // TODO remove
        cid = strtoul(optarg, &end_ptr, 10);
        if ((end_ptr == NULL) || (*end_ptr != '\0'))
          return -4;
        if (server_or_client != -1)
          return -4;
        server_or_client = 1;
        break;
      case 'h': // ctrl_hostname
        // TODO remove
        ctrl_hostname = (char *)malloc((strlen(optarg) + 1) * sizeof(char));
        strcpy(ctrl_hostname, optarg);
        break;
      case 'H': // ctrl_host
        // TODO remove
        hostspath = (char *)malloc((strlen(optarg) + 1) * sizeof(char));
        strcpy(hostspath, optarg);
        break;
      case 'i': // ctrl_init
        // TODO remove
        ctrl_init = (char *)malloc((strlen(optarg) + 1) * sizeof(char));
        strcpy(ctrl_init, optarg);
        break;
      case 'k':
        // TODO remove
        ctrl_key = (char *)malloc((strlen(optarg) + 1) * sizeof(char));
        strcpy(ctrl_key, optarg);
        break;
      case 'r': // logging path
        // TODO remove
        logging_path = string(optarg);
        break;
      case 'p':
        // TODO remove
        ctrl_port = strtoul(optarg, &end_ptr, 10);
        if ((end_ptr == NULL) || (*end_ptr != '\0')) return -4;
        break;

      // case 'r':
      //    ctrl_run = (char *)malloc((strlen(optarg) + 1) * sizeof(char));
      //    strcpy(ctrl_run, optarg);
      //    break;
      case 's': // site id
        // TODO remove
        sid = strtoul(optarg, &end_ptr, 10);

        if ((end_ptr == NULL) || (*end_ptr != '\0')) return -4;

        if (server_or_client != -1) return -4;
        server_or_client = 0;
        break;
      case 'S': // client touch only single server
      {
        // TODO remove
        int single_server_buf = strtoul(optarg, &end_ptr, 10);

        if ((end_ptr == NULL) || (*end_ptr != '\0')) return -4;

        switch (single_server_buf) {
          case 0:
            single_server = SS_DISABLED;
            break;
          case 1:
            single_server = SS_THREAD_SINGLE;
            break;
          case 2:
            single_server = SS_PROCESS_SINGLE;
            break;
          default:
            return -4;
        }
        break;
      }
      case '?':
        // TODO remove
        if ((optopt == 'c') ||
            (optopt == 'd') ||
            (optopt == 'f') ||
            (optopt == 'h') ||
            (optopt == 'i') ||
            (optopt == 'k') ||
            (optopt == 'p') ||
            (optopt == 'r') ||
            (optopt == 's') ||
            (optopt == 't')) Log_error("Option -%c requires an argument.",
                                       optopt);
        else if (isprint(optopt)) Log_error("Unknown option -%c.", optopt);
        else Log_error("Unknown option \\x%x", optopt);
        return -2;
      default:
        return -3;
    }
  }

//  if ((server_or_client != 0) && (server_or_client != 1)) return -5;
  verify(config_s == nullptr);
  config_s = new Config(
    ctrl_hostname,
    ctrl_port,
    ctrl_timeout,
    ctrl_key,
    ctrl_init,

    // ctrl_run,
    duration,
    heart_beat,
    single_server,
    logging_path);
  config_s->proc_name_ = proc_name;
  config_s->config_paths_ = config_paths;
  config_s->Load();
  return 0;
}

void Config::DestroyConfig() {
  if (config_s) {
    delete config_s;
    config_s = NULL;
  }
}

//Config::Config() {}

Config::Config(char           *ctrl_hostname,
               uint32_t        ctrl_port,
               uint32_t        ctrl_timeout,
               char           *ctrl_key,
               char           *ctrl_init,
               uint32_t        duration,
               bool            heart_beat,
               single_server_t single_server,
               string           logging_path) :
  heart_beat_(heart_beat),
  ctrl_hostname_(ctrl_hostname),
  ctrl_port_(ctrl_port),
  ctrl_timeout_(ctrl_timeout),
  ctrl_key_(ctrl_key),
  ctrl_init_(ctrl_init),
  duration_(duration),
  config_paths_(vector<string>()),
  mode_(0),
  proc_id_(0),
  benchmark_(0),
  scale_factor_(1),
  txn_weight_(vector<double>()),
  txn_weights_(map<string, double>()),
  proc_name_(string()),
  batch_start_(false),
  early_return_(false),
  retry_wait_(false),
  logging_path_(logging_path),
  single_server_(single_server),
  concurrent_txn_(1),
  max_retry_(1),
  num_site_(0),
  start_coordinator_id_(0),
  site_(vector<string>()),
  site_threads_(vector<uint32_t>()),
  num_coordinator_threads_(1),
  sid_(1),
  cid_(1),
  next_site_id_(0),
  proc_host_map_(map<string, string>()),
  sharding_(nullptr)
{
}

void Config::Load() {
  for (auto &name: config_paths_) {
    // XML configurations are deprecated
//    if (boost::algorithm::ends_with(name, "xml")) {
//      LoadXML(name);
//    }
    if (boost::algorithm::ends_with(name, "yml")) {
      LoadYML(name);
    } else {
      verify(0);
    }
  }

  // TODO particular configuration for certain workloads.
  this->InitTPCCD();
}

void Config::LoadYML(std::string &filename) {
//  YAML::Node config = YAML::LoadFile(name);

  YAML::Node config = YAML::LoadFile(filename);

//  verify(Sharding::sharding_s);

//  Sharding::sharding_s = new Sharding();

  if (config["site"]) {
    LoadSiteYML(config["site"]);
  }
  if (config["process"]) {
    LoadProcYML(config["process"]);
  }
  if (config["host"]) {
    LoadHostYML(config["host"]);
  }
  if (config["mode"]) {
    LoadModeYML(config["mode"]);
  }
  if (config["bench"]) {
    LoadBenchYML(config["bench"]);
  }
  if (config["schema"]) {
    LoadSchemaYML(config["schema"]);
  }
  if (config["sharding"]) {
    LoadShardingYML(config["sharding"]);
  }
}

void Config::LoadSiteYML(YAML::Node config) {
  auto servers = config["server"];
  int partition_id = 0;
  int site_id = 0;
  for (auto server_it = servers.begin(); server_it != servers.end(); server_it++) {
    auto group = *server_it;
    ReplicaGroup replica_group(partition_id);
    for (auto group_it = group.begin(); group_it != group.end(); group_it++) {
      auto site_addr = group_it->as<string>();
      SiteInfo info(site_id++, site_addr);
      info.type_ = SERVER;
      replica_group.replicas.push_back(info);
    }
    replica_groups_.push_back(replica_group);
    partition_id++;
  }

  auto clients = config["client"];
  for (auto client_it = clients.begin(); client_it != clients.end(); client_it++) {
    auto group = *client_it;
    vector<string> v;
    for (auto group_it = group.begin(); group_it != group.end(); group_it++) {
      auto site_name = group_it->as<string>();
      SiteInfo info(site_id++);
      info.name = site_name;
      info.type_ = CLIENT;
      par_clients_.push_back(info);
    }
  }
}

void Config::LoadProcYML(YAML::Node config) {
  for (auto it = config.begin(); it != config.end(); it++) {
    auto site_name = it->first.as<string>();
    auto proc_name = it->second.as<string>();
    auto info = SiteByName(site_name);
    verify(info != nullptr);
    info->proc_name = proc_name;
  }
}

void Config::LoadHostYML(YAML::Node config) {
  for (auto it = config.begin(); it != config.end(); it++) {
    auto proc_name = it->first.as<string>();
    auto host_name = it->second.as<string>();
    proc_host_map_[proc_name] = host_name;
    for (auto& group : replica_groups_) {
      for (auto& server : group.replicas) {
        if (server.proc_name == proc_name) {
          server.host = host_name;
        }
      }
    }
    for (auto& client : par_clients_) {
        if (client.proc_name == proc_name) {
          client.host = host_name;
        }
    }
  }
}

void Config::init_mode(std::string& mode_str) {
  auto it = modes_map_.find(mode_str);

  if (it != modes_map_.end()) {
    mode_ = it->second;
  } else {
    verify(0);
  }

  if ((mode_str == "rcc") || (mode_str == "deptran")) {
    // deprecated
    early_return_ = false;
  } else if (mode_str == "deptran_er") {
    // deprecated
    early_return_ = true;
  } else if (mode_str == "2pl_w") {
    retry_wait_ = true;
  } else if (mode_str == "2pl_wait_die") {
    mdb::FineLockedRow::set_wait_die();
  } else if ((mode_str == "2pl_ww") || (mode_str == "2pl_wound_die")) {
    mdb::FineLockedRow::set_wound_die();
  } 
}

void Config::init_bench(std::string& bench_str) {
  if (bench_str == "tpca") {
    benchmark_ = TPCA;
  } else if (bench_str == "tpcc") {
    benchmark_ = TPCC;
  } else if (bench_str == "tpcc_dist_part") {
    benchmark_ = TPCC_DIST_PART;
  } else if (bench_str == "tpccd") {
    benchmark_ = TPCC_REAL_DIST_PART;
  } else if (bench_str == "tpcc_real_dist_part") {
    benchmark_ = TPCC_REAL_DIST_PART;
  } else if (bench_str == "rw_benchmark") {
    benchmark_ = RW_BENCHMARK;
  } else if (bench_str == "micro_bench") {
    benchmark_ = MICRO_BENCH;
  } else if (bench_str == "simple_bench") {
    benchmark_ = SIMPLE_BENCH;
  } else {
    Log_error("No implementation for benchmark: %s", bench_str.c_str());
    verify(0);
  }
}

std::string Config::site2host_addr(std::string& siteaddr) {
  auto pos = siteaddr.find_first_of(':');

  verify(pos != std::string::npos);
  std::string sitename = siteaddr.substr(0, pos);
  std::string hostname = site2host_name(sitename);
  std::string hostaddr = siteaddr.replace(0, pos, hostname);
  return hostaddr;
}

std::string Config::site2host_name(std::string& sitename) {
  //    Log::debug("find host name by site name: %s", sitename.c_str());
  auto it = proc_host_map_.find(sitename);

  if (it != proc_host_map_.end()) {
    return it->second;
  } else {
    return sitename;
  }
}

void Config::LoadModeYML(YAML::Node config) {
  auto mode_str = config["cc"].as<string>();
  boost::algorithm::to_lower(mode_str);
  this->init_mode(mode_str);
  max_retry_ = config["retry"].as<int>();
  concurrent_txn_ = config["ongoing"].as<int>();
  batch_start_ = config["batch"].as<bool>();
}

void Config::LoadBenchYML(YAML::Node config) {
  std::string bench_str = config["workload"].as<string>();
  this->init_bench(bench_str);
  scale_factor_ = config["scale"].as<int>();
  auto weights = config["weight"];
  for (auto it = weights.begin(); it != weights.end(); it++) {
    auto txn_name = it->first.as<string>();
    auto weight = it->second.as<double>();
    txn_weights_[txn_name] = weight;
  }

  txn_weight_.push_back(txn_weights_["new_order"]);
  txn_weight_.push_back(txn_weights_["payment"]);
  txn_weight_.push_back(txn_weights_["order_status"]);
  txn_weight_.push_back(txn_weights_["delivery"]);
  txn_weight_.push_back(txn_weights_["stock_level"]);
//  this->InitTPCCD();
  sharding_ = Frame().CreateSharding();
  auto populations = config["population"];
  auto &tb_infos = sharding_->tb_infos_;
  for (auto it = populations.begin(); it != populations.end(); it++) {
    auto tbl_name = it->first.as<string>();
    auto info_it = tb_infos.find(tbl_name);
    if(info_it == tb_infos.end()) {
      tb_infos[tbl_name] = Sharding::tb_info_t();
      info_it = tb_infos.find(tbl_name);
    }
    auto &tbl_info = info_it->second;
    int pop = it->second.as<int>();
    tbl_info.num_records = scale_factor_ * pop;
    verify(tbl_info.num_records > 0);
  }
}

void Config::LoadSchemaYML(YAML::Node config) {
  verify(sharding_);
  auto &tb_infos = sharding_->tb_infos_;
  for (auto it = config.begin(); it != config.end(); it++) {
    auto table_node = *it;
    std::string tbl_name = table_node["name"].as<string>();

    auto info_it = tb_infos.find(tbl_name);
    if(info_it == tb_infos.end()) {
      tb_infos[tbl_name] = Sharding::tb_info_t();
      info_it = tb_infos.find(tbl_name);
    }
    auto &tbl_info = info_it->second;
    auto columns = table_node["column"];
    for (auto iitt = columns.begin(); iitt != columns.end(); iitt++) {
      auto column = *iitt;
      LoadSchemaTableColumnYML(tbl_info, column);
    }

    tbl_info.tb_name = tbl_name;
    sharding_->tb_infos_[tbl_name] = tbl_info;
  }
  sharding_->BuildTableInfoPtr();
}

void Config::LoadSchemaTableColumnYML(Sharding::tb_info_t &tb_info,
                                      YAML::Node column) {
  std::string c_type = column["type"].as<string>();
  verify(c_type.size() > 0);
  Value::kind c_v_type;

  if (c_type == "i32" || c_type == "integer") {
    c_v_type = Value::I32;
  } else if (c_type == "i64") {
    c_v_type = Value::I64;
  } else if (c_type == "double") {
    c_v_type = Value::DOUBLE;
  } else if (c_type == "str" || c_type == "string") {
    c_v_type = Value::STR;
  } else {
    c_v_type = Value::UNKNOWN;
    verify(0);
  }

  std::string c_name = column["name"].as<string>();
  verify(c_name.size() > 0);

  bool c_primary = column["primary"].as<bool>(false);

  std::string c_foreign = column["foreign"].as<string>("");
  Sharding::column_t  *foreign_key_column = NULL;
  Sharding::tb_info_t *foreign_key_tb     = NULL;
  std::string ftbl_name;
  std::string fcol_name;
  bool is_foreign = (c_foreign.size() > 0);
  if (is_foreign) {
    size_t pos = c_foreign.find('.');
    verify(pos != std::string::npos);

    ftbl_name = c_foreign.substr(0, pos);
    fcol_name = c_foreign.substr(pos + 1);
    verify(c_foreign.size() > pos + 1);
  }
  tb_info.columns.push_back(Sharding::column_t(c_v_type,
                                               c_name,
                                               c_primary,
                                               is_foreign,
                                               ftbl_name,
                                               fcol_name));
}

void Config::LoadShardingYML(YAML::Node config) {
  verify(sharding_);
  auto &tb_infos = sharding_->tb_infos_;
  for (auto it = config.begin(); it != config.end(); it++) {
    auto tbl_name = it->first.as<string>();
    auto info_it = tb_infos.find(tbl_name);
    verify(info_it != tb_infos.end());
    auto &tbl_info = info_it->second;
    string method = it->second.as<string>();

    tbl_info.num_site = get_num_site();
    for (auto replica_group_it = this->replica_groups_.begin();
         replica_group_it != this->replica_groups_.end();
         replica_group_it++) {
      auto &replica_group = *replica_group_it;
      for (auto replica_it = replica_group.replicas.begin();
           replica_it != replica_group.replicas.end();
           replica_it++) {
        const auto& replica = *replica_it;
        tbl_info.site_id.push_back(replica.id);
      }

      verify(tbl_info.num_site > 0 && tbl_info.num_site <= get_num_site());
      tbl_info.symbol = tbl_types_map_["sorted"];
    }
    verify(tbl_info.num_site > 0 && tbl_info.num_site <= get_num_site());
    // set tb_info.symbol TBL_SORTED or TBL_UNSORTED or TBL_SNAPSHOT
    tbl_info.symbol = tbl_types_map_["sorted"];
  }
}

void Config::InitTPCCD() {
  // TODO particular configuration for certain workloads.
  auto &tb_infos = sharding_->tb_infos_;
  if (benchmark_ == TPCC_REAL_DIST_PART) {
    i32 n_w_id =
        (i32)(tb_infos[std::string(TPCC_TB_WAREHOUSE)].num_records);
    verify(n_w_id > 0);
    i32 n_d_id = (i32)(get_num_site() *
        tb_infos[std::string(TPCC_TB_DISTRICT)].num_records / n_w_id);
    i32 d_id = 0, w_id = 0;

    for (d_id = 0; d_id < n_d_id; d_id++)
      for (w_id = 0; w_id < n_w_id; w_id++) {
        MultiValue mv(std::vector<Value>({Value(d_id),
                                          Value(w_id)}));
        sharding_->insert_dist_mapping(mv);
      }
    i32 n_i_id =
        (i32)(tb_infos[std::string(TPCC_TB_ITEM)].num_records);
    i32 i_id = 0;

    for (i_id = 0; i_id < n_i_id; i_id++)
      for (w_id = 0; w_id < n_w_id; w_id++) {
        MultiValue mv(std::vector<Value>({
                                             Value(i_id),
                                             Value(w_id)
                                         }));
        sharding_->insert_stock_mapping(mv);
      }
  }
}

Config::~Config() {
  if (sharding_) {
    delete sharding_;
    sharding_ = NULL;
  }

  if (ctrl_hostname_) {
    free(ctrl_hostname_);
    ctrl_hostname_ = NULL;
  }

  if (ctrl_key_) {
    free(ctrl_key_);
    ctrl_key_ = NULL;
  }

  if (ctrl_init_) {
    free(ctrl_init_);
    ctrl_init_ = NULL;
  }
}

unsigned int Config::get_site_id() {
  verify(0);
  return sid_;
}

unsigned int Config::get_client_id() {
  verify(0);
  return cid_;
}

unsigned int Config::get_ctrl_port() {
  return ctrl_port_;
}

unsigned int Config::get_ctrl_timeout() {
  return ctrl_timeout_;
}

const char * Config::get_ctrl_hostname() {
  return ctrl_hostname_;
}

const char * Config::get_ctrl_key() {
  return ctrl_key_;
}

const char * Config::get_ctrl_init() {
  return ctrl_init_;
}

// const char *Config::get_ctrl_run() {
//    return ctrl_run_;
// }
//
// TODO obsolete
int Config::get_all_site_addr(std::vector<std::string>& servers) {
  for (auto& site : GetMyServers()) {
    servers.push_back(site.GetHostAddr());
  }
  return servers.size();
}

int Config::get_site_addr(unsigned int sid, std::string& server) {
  // TODO
  verify(0);
  if (site_.size() == 0) verify(0);

  if (sid >= num_site_) verify(0);

  std::string siteaddr(site_[sid]);

  //    std::string hostaddr = site2host_addr(siteaddr);
  server.assign(siteaddr);
  return 0;
}

//int Config::get_my_addr(std::string& server) {
//  if (site_.size() == 0) verify(0);
//
//  if (sid_ >= num_site_) verify(0);
//
//  server.assign("0.0.0.0:");
//  uint32_t len = site_[sid_].length(), p_start = 0;
//  uint32_t port_pos = site_[sid_].find_first_of(':') + 1;
//  verify(p_start < len && p_start > 0);
//  server.append(site_[sid_].substr(port_pos));
//  return 0;
//}

int Config::get_threads(unsigned int& threads) {
  verify(0);
  if (site_threads_.size() == 0) return -1;

  if (sid_ >= num_site_) return -2;
  threads = site_threads_[sid_];
  return 0;
}

unsigned int Config::get_duration() {
  return duration_;
}

bool Config::do_heart_beat() {
  return heart_beat_;
}

int Config::get_mode() {
  return mode_;
}

unsigned int Config::get_num_threads() {
  return num_coordinator_threads_;
}

unsigned int Config::get_start_coordinator_id() {
  return start_coordinator_id_;
}

int Config::get_benchmark() {
  return benchmark_;
}

unsigned int Config::get_num_site() {
  return GetMyServers().size();
}

unsigned int Config::get_scale_factor() {
  return scale_factor_;
}

unsigned int Config::get_max_retry() {
  return max_retry_;
}

Config::single_server_t Config::get_single_server() {
  return single_server_;
}

unsigned int Config::get_concurrent_txn() {
  return concurrent_txn_;
}

bool Config::get_batch_start() {
  return batch_start_;
}

std::vector<double>& Config::get_txn_weight() {
  return txn_weight_;
}

#ifdef CPU_PROFILE
int Config::get_prof_filename(char *prof_file) {
  if (prof_file == NULL) return -1;
  return sprintf(prof_file, "log/site-%d.prof", sid_);
}

#endif // ifdef CPU_PROFILE

bool Config::do_early_return() {
  return early_return_;
}

bool Config::do_logging() {
  return logging_path_.empty();
}

const char * Config::log_path() {
  return logging_path_.c_str();
}

bool Config::retry_wait() {
  return retry_wait_;
}
}
