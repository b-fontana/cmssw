#ifndef HeterogeneousHGCalESProduct_h
#define HeterogeneousHGCalESProduct_h

#include <numeric> //accumulate
#include <typeinfo>

#include "HeterogeneousCore/CUDACore/interface/ESProduct.h"
#include "CUDADataFormats/HGCal/interface/HGCConditions.h"
#include "Geometry/HGCalCommonData/interface/HGCalDDDConstants.h"
#include "Geometry/HGCalCommonData/interface/HGCalParameters.h"
#include "UserCode/CodeGPU/plugins/KernelManagerHGCalRecHit.h"

namespace cp = hgcal_conditions::parameters;  

// Declare the wrapper ESProduct. The corresponding ESProducer should
// produce objects of this type.
class HeterogeneousConditionsESProductWrapper {
 public:
  // Constructor takes the standard CPU ESProduct, and transforms the
  // necessary data to array(s) in pinned host memory
  HeterogeneousConditionsESProductWrapper(const HGCalParameters*);
  
  // Deallocates all pinned host memory
  ~HeterogeneousConditionsESProductWrapper();
  
  // Function to return the actual payload on the memory of the current device
  hgcal_conditions::HeterogeneousHEFConditionsESProduct const *getHeterogeneousConditionsESProductAsync(cudaStream_t stream) const;

 private:
  // Holds the data in pinned CPU memory
  // Contrary to its non-heterogeneous counterpart (constructor argument) it is *not* a pointer to avoid an extra allocation
  cp::HeterogeneousHGCalParameters params_;

  std::vector<size_t> sizes_;
  size_t chunk_;

  void calculate_memory_bytes(const HGCalParameters*);
  double*& select_pointer_d(cp::HeterogeneousHGCalParameters*, const unsigned int&) const;
  std::vector<double> select_pointer_d(const HGCalParameters*, const unsigned int&) const;
  int32_t*& select_pointer_i(cp::HeterogeneousHGCalParameters*, const unsigned int&) const;
  std::vector<int32_t> select_pointer_i(const HGCalParameters*, const unsigned int&) const;

  // Helper struct to hold all information that has to be allocated and
  // deallocated per device
  struct GPUData {
    // Destructor should free all member pointers
    ~GPUData();
    // internal pointers are on device, struct itself is on CPU
    hgcal_conditions::HeterogeneousHEFConditionsESProduct *host = nullptr;
    // internal pounters and struct are on device
    hgcal_conditions::HeterogeneousHEFConditionsESProduct *device = nullptr;
  };

  // Helper that takes care of complexity of transferring the data to
  // multiple devices
  cms::cuda::ESProduct<GPUData> gpuData_;
};

#endif //HeterogeneousHGCalESProduct_h

