#include <cuda.h>
#include <cuda_runtime.h>
#include <inttypes.h>
#include "RecoLocalCalo/HGCalRecProducers/plugins/KernelManagerHGCalRecHit.h"
#include "RecoLocalCalo/HGCalRecProducers/plugins/HGCalRecHitKernelImpl.cuh"

KernelManagerHGCalRecHit::KernelManagerHGCalRecHit(HGCUncalibratedRecHitSoA* h_uncalibSoA,
                                                   HGCUncalibratedRecHitSoA* d_uncalibSoA,
                                                   HGCRecHitSoA* d_calibSoA)
    : h_uncalibSoA_(h_uncalibSoA), d_uncalibSoA_(d_uncalibSoA), d_calibSoA_(d_calibSoA) {
  nhits_ = h_uncalibSoA_->nhits_;
  stride_ = h_uncalibSoA_->stride_;
  ::nb_rechits_ = (stride_ + ::nt_rechits_.x - 1) / ::nt_rechits_.x;
  nbytes_device_ = d_uncalibSoA_->nbytes_ * stride_;
}

KernelManagerHGCalRecHit::KernelManagerHGCalRecHit(HGCRecHitSoA* h_calibSoA, HGCRecHitSoA* d_calibSoA)
    : h_calibSoA_(h_calibSoA), d_calibSoA_(d_calibSoA) {
  nhits_ = h_calibSoA_->nhits_;
  stride_ = h_calibSoA_->stride_;
  ::nb_rechits_ = (stride_ + ::nt_rechits_.x - 1) / ::nt_rechits_.x;
  nbytes_host_ = h_calibSoA_->nbytes_ * stride_;
}

KernelManagerHGCalRecHit::~KernelManagerHGCalRecHit() {}

void KernelManagerHGCalRecHit::transfer_soa_to_device_(const cudaStream_t& stream) {
  cudaCheck(cudaMemcpyAsync(
      d_uncalibSoA_->amplitude_, h_uncalibSoA_->amplitude_, nbytes_device_, cudaMemcpyHostToDevice, stream));
  cudaCheck(cudaGetLastError());
}

void KernelManagerHGCalRecHit::transfer_soa_to_host(const cudaStream_t& stream) {
  cudaCheck(cudaMemcpyAsync(h_calibSoA_->energy_, d_calibSoA_->energy_, nbytes_host_, cudaMemcpyDeviceToHost, stream));
  cudaCheck(cudaGetLastError());
}

void KernelManagerHGCalRecHit::run_kernels(const KernelConstantData<HGCeeUncalibratedRecHitConstantData>* kcdata,
                                           const cudaStream_t& stream) {
  transfer_soa_to_device_(stream);
  ee_to_rechit<<<::nb_rechits_, ::nt_rechits_, 0, stream>>>(*d_calibSoA_, *d_uncalibSoA_, kcdata->data_, nhits_);
  cudaCheck(cudaGetLastError());
}

void KernelManagerHGCalRecHit::run_kernels(const KernelConstantData<HGChefUncalibratedRecHitConstantData>* kcdata,
                                           const cudaStream_t& stream) {
  transfer_soa_to_device_(stream);
  hef_to_rechit<<<::nb_rechits_, ::nt_rechits_, 0, stream>>>(*d_calibSoA_, *d_uncalibSoA_, kcdata->data_, nhits_);
  cudaCheck(cudaGetLastError());
}

void KernelManagerHGCalRecHit::run_kernels(const KernelConstantData<HGChebUncalibratedRecHitConstantData>* kcdata,
                                           const cudaStream_t& stream) {
  transfer_soa_to_device_(stream);
  heb_to_rechit<<<::nb_rechits_, ::nt_rechits_, 0, stream>>>(*d_calibSoA_, *d_uncalibSoA_, kcdata->data_, nhits_);
  cudaCheck(cudaGetLastError());
}

HGCRecHitSoA* KernelManagerHGCalRecHit::get_output() { return h_calibSoA_; }
