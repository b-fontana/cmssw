#include "HeterogeneousHGCalEERecHitProducer.h"

HeterogeneousHGCalEERecHitProducer::HeterogeneousHGCalEERecHitProducer(const edm::ParameterSet& ps):
  token_(consumes<HGCUncalibratedRecHitCollection>(ps.getParameter<edm::InputTag>("HGCEEUncalibRecHitsTok")))
{
  cdata_.keV2DIGI_   = ps.getParameter<double>("HGCEE_keV2DIGI");
  cdata_.xmin_             = ps.getParameter<double>("minValSiPar"); //float
  cdata_.xmax_             = ps.getParameter<double>("maxValSiPar"); //float
  cdata_.aterm_            = ps.getParameter<double>("constSiPar"); //float
  cdata_.cterm_            = ps.getParameter<double>("noiseSiPar"); //float
  vdata_.fCPerMIP_         = ps.getParameter< std::vector<double> >("HGCEE_fCPerMIP");
  vdata_.cce_              = ps.getParameter<edm::ParameterSet>("HGCEE_cce").getParameter<std::vector<double> >("values");
  vdata_.noise_fC_         = ps.getParameter<edm::ParameterSet>("HGCEE_noise_fC").getParameter<std::vector<double> >("values");
  vdata_.rcorr_            = ps.getParameter< std::vector<double> >("rcorr");
  vdata_.weights_          = ps.getParameter< std::vector<double> >("weights");
  cdata_.uncalib2GeV_ = 1e-6 / cdata_.keV2DIGI_;

  assert_sizes_constants_(vdata_);

  tools_.reset(new hgcal::RecHitTools());

  produces<HGCeeRecHitCollection>(collection_name_);
}

void HeterogeneousHGCalEERecHitProducer::set_conditions_(const edm::EventSetup& setup)
{
  tools_->getEventSetup(setup);
  std::string handle_str;
  handle_str = "HGCalEESensitive";
  edm::ESHandle<HGCalGeometry> handle;
  setup.get<IdealGeometryRecord>().get(handle_str, handle);
  ddd_ = &( handle->topology().dddConstants() );
  params_ = ddd_->getParameter();
}

HeterogeneousHGCalEERecHitProducer::~HeterogeneousHGCalEERecHitProducer()
{
  delete kmdata_;
  delete kcdata_;
  delete uncalibSoA_;
  delete d_uncalibSoA_;
  delete d_intermediateSoA_;
  delete d_calibSoA_;
  delete calibSoA_;
}

std::string HeterogeneousHGCalEERecHitProducer::assert_error_message_(std::string var, const size_t& s)
{
  std::string str1 = "The '";
  std::string str2 = "' array must be at least of size ";
  std::string str3 = " to hold the configuration data.";
  return str1 + var + str2 + std::to_string(s) + str3;
}

void HeterogeneousHGCalEERecHitProducer::assert_sizes_constants_(const HGCConstantVectorData& vd)
{
  if( vdata_.fCPerMIP_.size() > maxsizes_constants::hef_fCPerMIP )
    cms::cuda::LogError("MaxSizeExceeded") << this->assert_error_message_("fCPerMIP", vdata_.fCPerMIP_.size());
  else if( vdata_.cce_.size() > maxsizes_constants::hef_cce )
    cms::cuda::LogError("MaxSizeExceeded") << this->assert_error_message_("cce", vdata_.cce_.size());
  else if( vdata_.noise_fC_.size() > maxsizes_constants::hef_noise_fC )
    cms::cuda::LogError("MaxSizeExceeded") << this->assert_error_message_("noise_fC", vdata_.noise_fC_.size());
  else if( vdata_.rcorr_.size() > maxsizes_constants::hef_rcorr ) 
    cms::cuda::LogError("MaxSizeExceeded") << this->assert_error_message_("rcorr", vdata_.rcorr_.size());
  else if( vdata_.weights_.size() > maxsizes_constants::hef_weights ) 
    cms::cuda::LogError("MaxSizeExceeded") << this->assert_error_message_("weights", vdata_.weights_.size());
}

void HeterogeneousHGCalEERecHitProducer::acquire(edm::Event const& event, edm::EventSetup const& setup, edm::WaitingTaskWithArenaHolder w) {
  const cms::cuda::ScopedContextAcquire ctx{event.streamID(), std::move(w), ctxState_};

  event.getByToken(token_, handle_ee_);
  const auto &hits_ee = *handle_ee_;

  unsigned int nhits = hits_ee.size();
  stride_ = ( (nhits-1)/32 + 1 ) * 32; //align to warp boundary
  allocate_memory_();

  set_conditions_(setup);
  HeterogeneousHGCalEEConditionsWrapper esproduct(params_);
  /*
  d_conds = esproduct.getHeterogeneousConditionsESProductAsync(ctx.stream());
  */
  d_conds = nullptr;

  kcdata_ = new KernelConstantData<HGCeeUncalibratedRecHitConstantData>(cdata_, vdata_);
  convert_constant_data_(kcdata_);
  convert_collection_data_to_soa_(hits_ee, uncalibSoA_, nhits);
  kmdata_ = new KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA>(nhits, stride_, uncalibSoA_, d_uncalibSoA_, d_intermediateSoA_, d_calibSoA_, calibSoA_);
  KernelManagerHGCalRecHit kernel_manager(kmdata_);
  kernel_manager.run_kernels(kcdata_);

  rechits_ = std::make_unique<HGCRecHitCollection>();
  convert_soa_data_to_collection_(*rechits_, calibSoA_, nhits);
}

void HeterogeneousHGCalEERecHitProducer::produce(edm::Event& event, const edm::EventSetup& setup)
{
  cms::cuda::ScopedContextProduce ctx{ctxState_}; //only for GPU to GPU producers
  event.put(std::move(rechits_), collection_name_);
}

void HeterogeneousHGCalEERecHitProducer::allocate_memory_()
{
  uncalibSoA_ = new HGCUncalibratedRecHitSoA();
  d_uncalibSoA_ = new HGCUncalibratedRecHitSoA();
  d_intermediateSoA_ = new HGCUncalibratedRecHitSoA();
  d_calibSoA_ = new HGCRecHitSoA();
  calibSoA_ = new HGCRecHitSoA();
  kcdata_ = new KernelConstantData<HGCeeUncalibratedRecHitConstantData>(cdata_, vdata_);

  //_allocate memory for hits on the host
  memory::allocation::host(stride_, uncalibSoA_, mem_in_);
  //_allocate memory for hits on the device
  memory::allocation::device(stride_, d_uncalibSoA_, d_intermediateSoA_, d_calibSoA_, d_mem_);
  //_allocate memory for hits on the host
  memory::allocation::host(stride_, calibSoA_, mem_out_);
}

void HeterogeneousHGCalEERecHitProducer::convert_constant_data_(KernelConstantData<HGCeeUncalibratedRecHitConstantData> *kcdata)
{
  for(size_t i=0; i<kcdata->vdata_.fCPerMIP_.size(); ++i)
    kcdata->data_.fCPerMIP_[i] = kcdata->vdata_.fCPerMIP_[i];
  for(size_t i=0; i<kcdata->vdata_.cce_.size(); ++i)
    kcdata->data_.cce_[i] = kcdata->vdata_.cce_[i];
  for(size_t i=0; i<kcdata->vdata_.noise_fC_.size(); ++i)
    kcdata->data_.noise_fC_[i] = kcdata->vdata_.noise_fC_[i];
  for(size_t i=0; i<kcdata->vdata_.rcorr_.size(); ++i)
    kcdata->data_.rcorr_[i] = kcdata->vdata_.rcorr_[i];
  for(size_t i=0; i<kcdata->vdata_.weights_.size(); ++i)
    kcdata->data_.weights_[i] = kcdata->vdata_.weights_[i];
}

void HeterogeneousHGCalEERecHitProducer::convert_collection_data_to_soa_(const edm::SortedCollection<HGCUncalibratedRecHit>& hits, HGCUncalibratedRecHitSoA* d, const unsigned int& nhits)
{
  for(unsigned int i=0; i<nhits; ++i)
    {
      d->amplitude_[i] = hits[i].amplitude();
      d->pedestal_[i] = hits[i].pedestal();
      d->jitter_[i] = hits[i].jitter();
      d->chi2_[i] = hits[i].chi2();
      d->OOTamplitude_[i] = hits[i].outOfTimeEnergy();
      d->OOTchi2_[i] = hits[i].outOfTimeChi2();
      d->flags_[i] = hits[i].flags();
      d->aux_[i] = 0;
      d->id_[i] = hits[i].id().rawId();
    }
}

void HeterogeneousHGCalEERecHitProducer::convert_soa_data_to_collection_(HGCRecHitCollection& rechits, HGCRecHitSoA *d, const unsigned int& nhits)
{
  rechits.reserve(nhits);
  for(uint i=0; i<nhits; ++i)
    {
      DetId id_converted( d->id_[i] );
      rechits.emplace_back( HGCRecHit(id_converted, d->energy_[i], d->time_[i], 0, d->flagBits_[i]) );
    }
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(HeterogeneousHGCalEERecHitProducer);
