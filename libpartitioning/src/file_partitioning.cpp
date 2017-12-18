#include <algorithm>
#include <iostream>
#include <iterator>

#include <file_partitioning.h>
#include <utils.h>

File_partitioner::File_partitioner(const Graph &graph, const Config &config)
    : Partitioner(0, graph, config), m_partitioning_epoch(0) {
  assert(!config.FILE_PATH.empty());
  m_partitioning_file = std::ifstream(config.FILE_PATH);

  uint32_t n_vertices, part;
  m_partitioning_file >> m_partitioning_epoch >> n_vertices;
  for (int i = 0; i < n_vertices; ++i) {
    m_partitioning_file >> part;
    m_partitioning[i] = part;
  }
  m_partitioning_file >> m_partitioning_epoch;
}

bool File_partitioner::trigger_partitioning(
    uint32_t new_timestamp, uint32_t cross_edge_access,
    uint32_t same_partition_edge_access) {
  if (new_timestamp == m_partitioning_epoch) {
    return true;
  }
  return false;
}

uint32_t File_partitioner::partition(int32_t n_partitions) {

  uint32_t n_vertices, part;
  auto old_partitioning = std::move(m_partitioning);
  assert(m_partitioning.size() == 0);

  m_partitioning_file >> n_vertices;
  for (int i = 0; i < n_vertices; ++i) {
    m_partitioning_file >> part;
    m_partitioning[i] = part;
  }
  m_partitioning_file >> m_partitioning_epoch;

  return 0;//calculate_movements_repartition(old_partitioning, n_partitions);
}

// // Hash partitioning for new vertexes
// void File_partitioner::assign_partition(const std::set<uint32_t>
// &vertex_list,
//                                       int32_t n_partitions) {
//   Utils::assign_hash_partition(m_partitioning, m_balance, vertex_list,
//                                n_partitions);
// }