#include <cassert>
#include <iomanip>
#include <metis.h>

#include <METIS_partitioning.h>
#include <log.h>
#include <partitioner.h>
#include <utils.h>

METIS_partitioner::METIS_partitioner(Graph &graph, Config &config)
    : Partitioner(METIS_SEED, graph, config) {
  assert(m_seed > 0);
  METIS_SetDefaultOptions(METIS_OPTIONS);
  METIS_OPTIONS[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_VOL;
  // METIS_OPTIONS[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
  METIS_OPTIONS[METIS_OPTION_SEED] = m_seed;
  METIS_OPTIONS[METIS_OPTION_UFACTOR] = 300;
  // METIS_OPTIONS[METIS_OPTION_DBGLVL] = 2;
}

uint32_t
METIS_partitioner::partition(const Graph &graph,
                             const std::map<uint32_t, Vertex> &id_to_vertex,
                             std::unordered_map<uint32_t, uint32_t> &partitioning,
                             idx_t nparts) {
  LOG_INFO("Begin partitioning");
  auto weight_map = boost::get(boost::edge_weight, graph);

  std::unordered_map<uint32_t, idx_t> to_metis_vtx;
  std::unordered_map<idx_t, uint32_t> from_metis_vtx;
  uint32_t next_id = 0;
  auto get_metis_id = [&](uint32_t vtx) {
    if (to_metis_vtx.count(vtx) == 0) {
      to_metis_vtx[vtx] = next_id;
      from_metis_vtx[next_id] = vtx;
      ++next_id;
    }
    return to_metis_vtx[vtx];
  };

  out_edge_it edg_it, edg_it_end;
  Edge ed;
  // void partition_METIS(Graph &g, idx_t nparts) {
  std::pair<vertex_it, vertex_it> vertex;

  idx_t nvtxs = boost::num_vertices(graph);
  if (!nvtxs)
    return 0;
  idx_t ncon = 1;
  idx_t *xadj = (idx_t *)malloc((nvtxs + 1) * sizeof(idx_t));
  idx_t *adjncy = (idx_t *)malloc(2 * boost::num_edges(graph) * sizeof(idx_t));
  idx_t *adjwgt = (idx_t *)malloc(2 * boost::num_edges(graph) * sizeof(idx_t));
  idx_t *vwgt = (idx_t *)malloc(ncon * nvtxs * sizeof(idx_t));
  idx_t *part = (idx_t *)malloc(nvtxs * sizeof(idx_t));

  idx_t edge_idx = 0;
  xadj[0] = 0;
  idx_t v_idx = 1;

  std::vector<std::vector<std::pair<idx_t, idx_t>>> transformed_graph(nvtxs);

  // for (vertex = boost::vertices(graph); vertex.first != vertex.second;
  //    ++vertex.first) {
  auto before = std::chrono::high_resolution_clock::now();
  for (const auto &vtx_k_v : id_to_vertex) {
    auto vertex_id = get_metis_id(Utils::get_id(vtx_k_v.second, graph));
    vwgt[vertex_id] = graph[vtx_k_v.second].m_vertex_weight;
    std::vector<std::pair<uint32_t, uint32_t>> neighboors;
    for (tie(edg_it, edg_it_end) = boost::out_edges(vtx_k_v.second, graph);
         edg_it != edg_it_end; ++edg_it) {
      neighboors.push_back({Utils::get_id(boost::target(*edg_it, graph), graph),
                            get(weight_map, *edg_it)});
    }
    sort(neighboors.begin(), neighboors.end());
    for (const auto &neighboor : neighboors) {
      auto neighboor_id = get_metis_id(neighboor.first);
      transformed_graph[vertex_id].push_back({neighboor_id, neighboor.second});
    }
  }
  assert(transformed_graph.size() == nvtxs);
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 (std::chrono::high_resolution_clock::now() - before))
                 .count();
  LOG_INFO("Time to construct metagraph: %lld", now);
  before = std::chrono::high_resolution_clock::now();
  for (idx_t vertex = 0; vertex < transformed_graph.size(); ++vertex, ++v_idx) {
    for (auto const &neighboor : transformed_graph[vertex]) {
      adjncy[edge_idx] = neighboor.first;
      adjwgt[edge_idx] = neighboor.second;
      ++edge_idx;
    }
    xadj[v_idx] = edge_idx;
  }
  now = std::chrono::duration_cast<std::chrono::milliseconds>(
            (std::chrono::high_resolution_clock::now() - before))
            .count();
  LOG_INFO("Time to construct METIS graph: %lld", now);
  before = std::chrono::high_resolution_clock::now();

  idx_t objval;
  int return_METIS = METIS_PartGraphKway(
      &nvtxs,  // The number of vertices in the graph.
      &ncon,   // The number of balancing constraints
      xadj,    // The adjacency structure of the graph
      adjncy,  // The adjacency structure of the graph
      NULL,    // The weights of the vertices
      NULL,    // Size of vertices for computing the total communication volume
      adjwgt,  // The weights of the edges
      &nparts, // The number of parts to partition the graph
      NULL, // nparts×ncon that specifies the desired weight for each partition
      NULL, // array of size ncon that specifies the allowed load imbalance
            // tolerance for each constraint
      METIS_OPTIONS, // METIS options
      &objval, part);
  assert(return_METIS == METIS_OK);

  now = std::chrono::duration_cast<std::chrono::milliseconds>(
            (std::chrono::high_resolution_clock::now() - before))
            .count();
  LOG_INFO("Time to run METIS: %lld", now);
  before = std::chrono::high_resolution_clock::now();
  
  auto old_partitioning = std::move(partitioning);
  assert(partitioning.size() == 0);
  assert(old_partitioning.size() == nvtxs);
  for (int i = 0; i < nvtxs; ++i) {
    partitioning[from_metis_vtx[i]] = part[i];
  }

  free(xadj);
  free(adjncy);
  free(adjwgt);
  free(vwgt);
  free(part);

  return calculate_movements_repartition(old_partitioning, partitioning, nparts);
}
std::string METIS_partitioner::get_name() {

  std::stringstream stream;
  stream << std::fixed << std::setprecision(2)
         << m_config.CROSS_PARTITION_THRESHOLD;
  std::string threshold = stream.str();

  std::string METIS_mode =
      "METIS_" +
      ((m_config.PARTITIONING_TYPE == Config::PERIODIC_PARTITIONING)
           ? "PERIODIC_"
           : "DYNAMIC_" + threshold + "_WINDOW_" +
                 std::to_string(m_config.TIME_REPARTITION_WINDOW) + "_");

  return METIS_mode + "repart_" + std::to_string(m_config.TIME_REPARTITION) +
         "_seed_" + std::to_string(METIS_SEED);
}