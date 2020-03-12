#include <fstream>
#include <string>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/log/trivial.hpp>
#include <boost/serialization/vector.hpp>

#include <graphtyper/graph/haplotype_calls.hpp>


namespace gyper
{


//
/// HaplotypeCalls
//
HaplotypeCall::HaplotypeCall(Haplotype const & hap)
{
  calls = hap.get_haplotype_calls(haplotype_impurity);
  num_samples = hap.hap_samples.size();
  gts = hap.gts;
  assert(hap.var_stats.size() == hap.gts.size());

  long const NUM_GTS = hap.gts.size();

  // Determine bias
  read_strand.reserve(hap.gts.size());

  for (long g = 0; g < NUM_GTS; ++g)
  {
    auto const & var_stat = hap.var_stats[g];
    read_strand.push_back(var_stat.read_strand);
  }
}


void
HaplotypeCall::merge_with(HaplotypeCall const & other)
{
  assert(other.calls.size() >= 1);
  assert(other.calls[0] == 0);
  std::copy(other.calls.begin() + 1, other.calls.end(), std::back_inserter(calls));
  num_samples += other.num_samples;

  assert(haplotype_impurity.size() == other.haplotype_impurity.size());

  for (long i = 0; i < static_cast<long>(haplotype_impurity.size()); ++i)
    haplotype_impurity[i] += other.haplotype_impurity[i];

  assert(read_strand.size() == other.read_strand.size());

  for (long i = 0; i < static_cast<long>(read_strand.size()); ++i)
  {
    auto & rs = read_strand[i];
    auto const & other_rs = other.read_strand[i];
    assert(rs.size() == other_rs.size());

    for (long j = 0; j < static_cast<long>(rs.size()); ++j)
      rs[j].merge_with(other_rs[j]);
  }
}


void
HaplotypeCall::make_calls_unique()
{
  std::sort(calls.begin(), calls.end());
  calls.erase(std::unique(calls.begin(), calls.end()), calls.end());
}


template <class Archive>
void
HaplotypeCall::serialize(Archive & ar, unsigned const int /*version*/)
{
  ar & calls;
  ar & haplotype_impurity;
  ar & num_samples;
  ar & gts;
  ar & read_strand;
}


//
/// HaplotypeCalls
//
HaplotypeCalls::HaplotypeCalls(std::vector<HaplotypeCall> const & _hap_calls)
  : hap_calls(_hap_calls)
{}


template <typename Archive>
void
HaplotypeCalls::serialize(Archive & ar, unsigned const int /*version*/)
{
  ar & hap_calls;
}


/***************************
 * EXPLICIT INSTANTIATIONS *
 ***************************/

template void HaplotypeCalls::serialize<boost::archive::binary_iarchive>(boost::archive::binary_iarchive &,
                                                                         const unsigned int);
template void HaplotypeCalls::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive &,
                                                                         const unsigned int);

/*********************************
 * FUNCTIONS TO MAKE LIFE EASIER *
 *********************************/

void
save_calls(HaplotypeCalls & calls, std::string const & filename)
{
  std::ofstream ofs(filename.c_str(), std::ios::binary);

  if (!ofs.is_open())
  {
    BOOST_LOG_TRIVIAL(error) << "[graphtyper::haplotype_calls] Could not save calls to location '"
                             << filename
                             << "'";
    std::exit(1);
  }

  boost::archive::binary_oarchive oa(ofs);
  oa << calls;
}


std::vector<HaplotypeCall>
load_calls(std::string filename)
{
  HaplotypeCalls calls{};
  std::ifstream ifs(filename.c_str(), std::ios::binary);

  if (!ifs.is_open())
  {
    BOOST_LOG_TRIVIAL(error) << "Could not open haplotype calls file " << filename;
    std::exit(1);
  }

  boost::archive::binary_iarchive ia(ifs);
  ia >> calls;
  return calls.get_hap_calls();
}


} // namespace gyper
