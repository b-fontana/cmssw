#ifndef CUDADAtaFormats_HGCal_HGCUncalibRecHitHost_H
#define CUDADAtaFormats_HGCal_HGCUncalibRecHitHost_H

#include <cassert>
#include <numeric>

#include "HeterogeneousCore/CUDAUtilities/interface/host_unique_ptr.h"
#include "CUDADataFormats/HGCal/interface/HGCRecHitSoA.h"
#include "CUDADataFormats/HGCal/interface/HGCUncalibRecHitSoA.h"

#include "RecoLocalCalo/HGCalRecProducers/interface/MessageDefinition.pb.h"

template <class T>
class HGCUncalibRecHitHost {
public:
  HGCUncalibRecHitHost() = default;
  explicit HGCUncalibRecHitHost(uint32_t nhits, const T& hits, const cudaStream_t& stream) : nhits_(nhits) {
    constexpr std::array<int,memory::npointers::ntypes_hgcuncalibrechits_soa> sizes = {{ memory::npointers::float_hgcuncalibrechits_soa * sizeof(float),
											 memory::npointers::uint32_hgcuncalibrechits_soa * sizeof(uint32_t) }};

    size_tot_ = std::accumulate(sizes.begin(), sizes.end(), 0);
    pad_ = ((nhits - 1) / 32 + 1) * 32;  //align to warp boundary (assumption: warpSize = 32)
    ptr_ = cms::cuda::make_host_unique<std::byte[]>(pad_ * size_tot_, stream);

    defineSoAMemoryLayout_();
    fillSoA_(hits);
  }
  ~HGCUncalibRecHitHost() = default;

  HGCUncalibRecHitHost(const HGCUncalibRecHitHost &) = delete;
  HGCUncalibRecHitHost &operator=(const HGCUncalibRecHitHost &) = delete;
  HGCUncalibRecHitHost(HGCUncalibRecHitHost &&) = default;
  HGCUncalibRecHitHost &operator=(HGCUncalibRecHitHost &&) = default;

  void defineSoAMemoryLayout_() {
    soa_.amplitude_    = reinterpret_cast<float*>(ptr_.get());
    soa_.pedestal_     = soa_.amplitude_ + pad_;
    soa_.jitter_       = soa_.pedestal_ + pad_;
    soa_.chi2_         = soa_.jitter_ + pad_;
    soa_.OOTamplitude_ = soa_.chi2_ + pad_;
    soa_.OOTchi2_      = soa_.OOTamplitude_ + pad_;
    soa_.flags_        = reinterpret_cast<uint32_t*>(soa_.OOTchi2_ + pad_);
    soa_.aux_          = soa_.flags_ + pad_;
    soa_.id_           = soa_.aux_ + pad_;
    soa_.aux_          = soa_.flags_ + pad_;
    soa_.id_           = soa_.aux_ + pad_;

    soa_.nbytes_ = size_tot_;
    soa_.nhits_  = nhits_;
    soa_.pad_   = pad_;
  }

  void fillSoA_(const uncalibRecHitsProtocol::Event& c) {
    for (unsigned i(0); i<nhits_; ++i) {
      soa_.amplitude_[i]    = c.amplitude(i);
      soa_.pedestal_[i]     = c.pedestal(i);
      soa_.jitter_[i]       = c.jitter(i);
      soa_.chi2_[i]         = c.chi2(i);
      soa_.OOTamplitude_[i] = c.ootamplitude(i);
      soa_.OOTchi2_[i]      = c.ootchi2(i);
      soa_.flags_[i]        = c.flags(i);
      soa_.aux_[i]          = 0;
      soa_.id_[i]           = c.id(i);
    }
  }

  void fillSoA_(const HGCUncalibratedRecHitCollection& c) {
    for (unsigned i(0); i<nhits_; ++i) {
      soa_.amplitude_[i]    = c[i].amplitude();
      soa_.pedestal_[i]     = c[i].pedestal();
      soa_.jitter_[i]       = c[i].jitter();
      soa_.chi2_[i]         = c[i].chi2();
      soa_.OOTamplitude_[i] = c[i].outOfTimeEnergy();
      soa_.OOTchi2_[i]      = c[i].outOfTimeChi2();
      soa_.flags_[i]        = c[i].flags();
      soa_.aux_[i]          = 0;
      soa_.id_[i]           = c[i].id().rawId();
    }
  }

  HGCUncalibRecHitSoA get() const { return soa_; }
  uint32_t nHits() const { return nhits_; }
  uint32_t pad() const { return pad_; }
  uint32_t nBytes() const { return size_tot_; }

private:
  cms::cuda::host::unique_ptr<std::byte[]> ptr_;
  HGCUncalibRecHitSoA soa_;
  uint32_t pad_;
  uint32_t nhits_;
  uint32_t size_tot_;
};

#endif  //CUDADAtaFormats_HGCal_HGCUncalibRecHitHost_H
