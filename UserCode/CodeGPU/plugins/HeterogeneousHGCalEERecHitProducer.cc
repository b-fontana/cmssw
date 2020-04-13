#include "HeterogeneousHGCalEERecHitProducer.h"

HeterogeneousHGCalEERecHitProducer::HeterogeneousHGCalEERecHitProducer(const edm::ParameterSet& ps):
  token_(consumes<HGCUncalibratedRecHitCollection>(ps.getParameter<edm::InputTag>("HGCEEUncalibRecHitsTok")))
{
  nhitsmax_                = ps.getParameter<uint32_t>("nhitsmax");
  cdata_.hgcEE_keV2DIGI_   = ps.getParameter<double>("HGCEE_keV2DIGI");
  cdata_.xmin_             = ps.getParameter<double>("minValSiPar"); //float
  cdata_.xmax_             = ps.getParameter<double>("maxValSiPar"); //float
  cdata_.aterm_            = ps.getParameter<double>("constSiPar"); //float
  cdata_.cterm_            = ps.getParameter<double>("noiseSiPar"); //float
  cdata_.rangeMatch_       = ps.getParameter<uint32_t>("rangeMatch");
  cdata_.rangeMask_        = ps.getParameter<uint32_t>("rangeMask");
  vdata_.fCPerMIP_         = ps.getParameter< std::vector<double> >("HGCEE_fCPerMIP");
  vdata_.cce_              = ps.getParameter<edm::ParameterSet>("HGCEE_cce").getParameter<std::vector<double> >("values");
  vdata_.noise_fC_         = ps.getParameter<edm::ParameterSet>("HGCEE_noise_fC").getParameter<std::vector<double> >("values");
  vdata_.rcorr_            = ps.getParameter< std::vector<double> >("rcorr");
  vdata_.weights_          = ps.getParameter< std::vector<double> >("weights");
  cdata_.s_hgcEE_fCPerMIP_ = vdata_.fCPerMIP_.size();
  cdata_.s_hgcEE_cce_      = vdata_.cce_.size();
  cdata_.s_hgcEE_noise_fC_ = vdata_.noise_fC_.size();
  cdata_.s_rcorr_          = vdata_.rcorr_.size();
  cdata_.s_weights_        = vdata_.weights_.size();
  cdata_.hgceeUncalib2GeV_ = 1e-6 / cdata_.hgcEE_keV2DIGI_;
  vdata_.waferTypeL_       = {0, 1, 2};//ddd_->retWaferTypeL(); if depends on geometry the allocation is tricky!
  cdata_.s_waferTypeL_     = vdata_.waferTypeL_.size();

  begin = std::chrono::steady_clock::now();

  tools_.reset(new hgcal::RecHitTools());
  stride_ = ( (nhitsmax_-1)/32 + 1 ) * 32; //align to warp boundary

  produces<HGCeeRecHitCollection>(collection_name_);
}

HeterogeneousHGCalEERecHitProducer::~HeterogeneousHGCalEERecHitProducer()
{
  delete kmdata_;
  delete h_kcdata_;
  delete d_kcdata_;
  delete old_soa_;
  delete d_oldhits_;
  delete d_newhits_;
  delete d_newhits_final_;
  delete h_newhits_;

  end = std::chrono::steady_clock::now();
  std::cout << "Time difference (heterogeneous) = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " [ms]" << std::endl;
}

void HeterogeneousHGCalEERecHitProducer::acquire(edm::Event const& event, edm::EventSetup const& setup, edm::WaitingTaskWithArenaHolder w) {
  const cms::cuda::ScopedContextAcquire ctx{event.streamID(), std::move(w), ctxState_};
  set_geometry_(setup);

  allocate_memory_();
  convert_constant_data_(h_kcdata_);

  event.getByToken(token_, handle_ee_);
  const auto &hits_ee = *handle_ee_;

  unsigned int nhits = hits_ee.size();
  convert_collection_data_to_soa_(hits_ee, old_soa_, nhits);

  kmdata_ = new KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>(nhitsmax_, stride_, old_soa_, d_oldhits_, d_newhits_, d_newhits_final_, h_newhits_);
  KernelManagerHGCalRecHit kernel_manager(kmdata_);
  kernel_manager.run_kernels(h_kcdata_, d_kcdata_);
  new_soa_ = kernel_manager.get_output();

  rechits_ = std::make_unique<HGCRecHitCollection>();
  convert_soa_data_to_collection_(*rechits_, new_soa_, nhits);
}

void HeterogeneousHGCalEERecHitProducer::produce(edm::Event& event, const edm::EventSetup& setup)
{
  cms::cuda::ScopedContextProduce ctx{ctxState_}; //only for GPU to GPU producers
  event.put(std::move(rechits_), collection_name_);
}

void HeterogeneousHGCalEERecHitProducer::allocate_memory_()
{
  old_soa_ = new HGCUncalibratedRecHitSoA();
  d_oldhits_ = new HGCUncalibratedRecHitSoA();
  d_newhits_ = new HGCUncalibratedRecHitSoA();
  d_newhits_final_ = new HGCRecHitSoA();
  h_newhits_ = new HGCRecHitSoA();
  h_kcdata_ = new KernelConstantData<HGCeeUncalibratedRecHitConstantData>(cdata_, vdata_);
  d_kcdata_ = new KernelConstantData<HGCeeUncalibratedRecHitConstantData>(cdata_, vdata_);

  //_allocate pinned memory for constants on the host
  memory::allocation::host(h_kcdata_, h_mem_const_);
  //_allocate pinned memory for constants on the device
  memory::allocation::device(d_kcdata_, d_mem_const_);
  //_allocate memory for hits on the host
  memory::allocation::host(nhitsmax_, old_soa_, h_mem_in_);
  //_allocate memory for hits on the device
  memory::allocation::device(nhitsmax_, d_oldhits_, d_newhits_, d_newhits_final_, d_mem_);
  //_allocate memory for hits on the host
  memory::allocation::host(nhitsmax_, h_newhits_, h_mem_out_);
}

void HeterogeneousHGCalEERecHitProducer::set_geometry_(const edm::EventSetup& setup)
{
  tools_->getEventSetup(setup);
  std::string handle_str;
  handle_str = "HGCalEESensitive";
  edm::ESHandle<HGCalGeometry> handle;
  setup.get<IdealGeometryRecord>().get(handle_str, handle);
  ddd_ = &(handle->topology().dddConstants());
}

void HeterogeneousHGCalEERecHitProducer::convert_constant_data_(KernelConstantData<HGCeeUncalibratedRecHitConstantData> *kcdata)
{
  for(int i=0; i<kcdata->data.s_hgcEE_fCPerMIP_; ++i)
    kcdata->data.hgcEE_fCPerMIP_[i] = kcdata->vdata.fCPerMIP_[i];
  for(int i=0; i<kcdata->data.s_hgcEE_cce_; ++i)
    kcdata->data.hgcEE_cce_[i] = kcdata->vdata.cce_[i];
  for(int i=0; i<kcdata->data.s_hgcEE_noise_fC_; ++i)
    kcdata->data.hgcEE_noise_fC_[i] = kcdata->vdata.noise_fC_[i];
  for(int i=0; i<kcdata->data.s_rcorr_; ++i)
    kcdata->data.rcorr_[i] = kcdata->vdata.rcorr_[i];
  for(int i=0; i<kcdata->data.s_weights_; ++i)
    kcdata->data.weights_[i] = kcdata->vdata.weights_[i];
  for(int i=0; i<kcdata->data.s_waferTypeL_; ++i)
    kcdata->data.waferTypeL_[i] = kcdata->vdata.waferTypeL_[i];
}

void HeterogeneousHGCalEERecHitProducer::convert_collection_data_to_soa_(const edm::SortedCollection<HGCUncalibratedRecHit>& hits, HGCUncalibratedRecHitSoA* d, const unsigned int& nhits)
{
  for(unsigned int i=0; i<nhits; ++i)
    {
      d->amplitude[i] = hits[i].amplitude();
      d->pedestal[i] = hits[i].pedestal();
      d->jitter[i] = hits[i].jitter();
      d->chi2[i] = hits[i].chi2();
      d->OOTamplitude[i] = hits[i].outOfTimeEnergy();
      d->OOTchi2[i] = hits[i].outOfTimeChi2();
      d->flags[i] = hits[i].flags();
      d->aux[i] = 0;
      d->id[i] = hits[i].id().rawId();
    }
}

void HeterogeneousHGCalEERecHitProducer::convert_soa_data_to_collection_(HGCRecHitCollection& rechits, HGCRecHitSoA *d, const unsigned int& nhits)
{
  rechits.reserve(nhits);
  for(uint i=0; i<nhits; ++i)
    {
      DetId id_converted( d->id[i] );
      rechits.emplace_back( HGCRecHit(id_converted, d->energy[i], d->time[i], 0, d->flagBits[i]) );
    }
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(HeterogeneousHGCalEERecHitProducer);
