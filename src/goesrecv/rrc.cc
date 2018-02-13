#include "rrc.h"

#include <cassert>
#include <cstring>

#include <arm_neon.h>

static float taps[][RRC::NTAPS] = {
  // Sample rate 3M, symbol rate 293883
  {
    -0.008763054385781288146973,
    -0.012546731159090995788574,
    -0.015255616977810859680176,
    -0.016073158010840415954590,
    -0.014261141419410705566406,
    -0.009271819144487380981445,
    -0.000846428214572370052338,
    0.010915086604654788970947,
    0.025525253266096115112305,
    0.042128123342990875244141,
    0.059566780924797058105469,
    0.076493091881275177001953,
    0.091507285833358764648438,
    0.103310413658618927001953,
    0.110850445926189422607422,
    0.113442927598953247070312,
    0.110850445926189422607422,
    0.103310413658618927001953,
    0.091507285833358764648438,
    0.076493091881275177001953,
    0.059566780924797058105469,
    0.042128123342990875244141,
    0.025525253266096115112305,
    0.010915086604654788970947,
    -0.000846428214572370052338,
    -0.009271819144487380981445,
    -0.014261141419410705566406,
    -0.016073158010840415954590,
    -0.015255616977810859680176,
    -0.012546731159090995788574,
    -0.008763054385781288146973,
  },

  // Sample rate 2.4M, symbol rate 293883
  {
    0.004133887588977813720703,
    0.000710888416506350040436,
    -0.004640835337340831756592,
    -0.010969181545078754425049,
    -0.016719518229365348815918,
    -0.019965615123510360717773,
    -0.018769890069961547851562,
    -0.011606029234826564788818,
    0.002245118841528892517090,
    0.022418288514018058776855,
    0.047378379851579666137695,
    0.074562914669513702392578,
    0.100740529596805572509766,
    0.122525796294212341308594,
    0.136953994631767272949219,
    0.142002552747726440429688,
    0.136953994631767272949219,
    0.122525796294212341308594,
    0.100740529596805572509766,
    0.074562914669513702392578,
    0.047378379851579666137695,
    0.022418288514018058776855,
    0.002245118841528892517090,
    -0.011606029234826564788818,
    -0.018769890069961547851562,
    -0.019965615123510360717773,
    -0.016719518229365348815918,
    -0.010969181545078754425049,
    -0.004640835337340831756592,
    0.000710888416506350040436,
    0.004133887588977813720703,
  },

  // Sample rate 3M, symbol rate 927000
  {
    0.002076130826026201248169,
    -0.000242356050875969231129,
    -0.003089555073529481887817,
    -0.000486591365188360214233,
    0.004652836360037326812744,
    0.003017527749761939048767,
    -0.004518595058470964431763,
    -0.003946761135011911392212,
    0.008618867956101894378662,
    0.010950570926070213317871,
    -0.017327804118394851684570,
    -0.048922080546617507934570,
    -0.015993200242519378662109,
    0.113865941762924194335938,
    0.276105165481567382812500,
    0.350479811429977416992188,
    0.276105165481567382812500,
    0.113865941762924194335938,
    -0.015993200242519378662109,
    -0.048922080546617507934570,
    -0.017327804118394851684570,
    0.010950570926070213317871,
    0.008618867956101894378662,
    -0.003946761135011911392212,
    -0.004518595058470964431763,
    0.003017527749761939048767,
    0.004652836360037326812744,
    -0.000486591365188360214233,
    -0.003089555073529481887817,
    -0.000242356050875969231129,
    0.002076130826026201248169,
  },

  // Sample rate 2.4M, symbol rate 927000
  {
    0.000990060623735189437866,
    -0.002150935120880603790283,
    -0.000491598912049084901810,
    0.002602360676974058151245,
    -0.001496038283221423625946,
    -0.003227273700758814811707,
    0.004611079115420579910278,
    0.003782370127737522125244,
    -0.007140222005546092987061,
    0.002135128714144229888916,
    0.016265124082565307617188,
    -0.021719822660088539123535,
    -0.061835423111915588378906,
    0.048223406076431274414062,
    0.299794435501098632812500,
    0.439314693212509155273438,
    0.299794435501098632812500,
    0.048223406076431274414062,
    -0.061835423111915588378906,
    -0.021719822660088539123535,
    0.016265124082565307617188,
    0.002135128714144229888916,
    -0.007140222005546092987061,
    0.003782370127737522125244,
    0.004611079115420579910278,
    -0.003227273700758814811707,
    -0.001496038283221423625946,
    0.002602360676974058151245,
    -0.000491598912049084901810,
    -0.002150935120880603790283,
    0.000990060623735189437866,
  },
};

RRC::RRC(int decimation, int sampleRate, int symbolRate) :
    decimation_(decimation) {
  int offset = 0;

  if (sampleRate == 3000000) {
    offset = 0;
  } else if (sampleRate == 2400000) {
    offset = 1;
  } else {
    assert(false);
  }

  if (symbolRate == 293883) {
    memcpy(taps_.data(), taps[0 + offset], NTAPS * sizeof(float));
  } else if (symbolRate == 927000) {
    memcpy(taps_.data(), taps[2 + offset], NTAPS * sizeof(float));
  } else {
    assert(false);
  }

  // Last tap is zero (so it is a multiple of 4)
  taps_[NTAPS] = 0.0f;

  // Seed the delay line with zeroes
  tmp_.resize(NTAPS);
}

void RRC::work(
    const std::shared_ptr<Queue<Samples> >& qin,
    const std::shared_ptr<Queue<Samples> >& qout) {
  auto input = qin->popForRead();
  if (!input) {
    qout->close();
    return;
  }

  auto output = qout->popForWrite();
  auto nsamples = input->size();
  assert((nsamples % decimation_) == 0);
  output->resize(nsamples / decimation_);
  tmp_.insert(tmp_.end(), input->begin(), input->end());
  assert(tmp_.size() == input->size() + NTAPS);

  // Return read buffer (it has been copied into tmp_)
  qin->pushRead(std::move(input));

  // Input/output cursors
  std::complex<float>* fi = tmp_.data();
  std::complex<float>* fo = output->data();

  // Below should work generically with libsimdpp, but fmadd
  // seems to be missing for NEON.
  //
  // // Load taps
  // simdpp::float32<4> taps[(NTAPS + 1) / 4];
  // for (size_t i = 0; i < (NTAPS + 1); i += 4) {
  //   taps[i / 4] = simdpp::load(&taps_[i]);
  // }
  //
  // for (size_t i = 0; i < nsamples; i += decimation_) {
  //   simdpp::float32<4> acci = simdpp::splat(0.0f);
  //   simdpp::float32<4> accq = simdpp::splat(0.0f);
  //
  //   // This can be unrolled
  //   for (size_t j = 0; j < ((NTAPS + 1) / 4); j++) {
  //     simdpp::float32<4> vali;
  //     simdpp::float32<4> valq;
  //     simdpp::load_packed2(vali, valq, &fi[j * 4]);
  //     acci = simdpp::fmadd(vali, taps[j], acci);
  //     accq = simdpp::fmadd(valq, taps[j], accq);
  //   }
  //
  //   fo[i].real(simdpp::reduce_add(acci));
  //   fo[i].imag(simdpp::reduce_add(accq));
  //
  //   // Advance input/output cursors
  //   fi += decimation_;
  //   fo += decimation_;
  // }

  // Load taps
  float32x4_t taps[(NTAPS + 1) / 4];
  for (size_t i = 0; i < (NTAPS + 1); i += 4) {
    taps[i / 4] = vld1q_f32(&taps_[i]);
  }

  for (size_t i = 0; i < (nsamples / decimation_); i++) {
    float32x4x2_t acc;
    acc.val[0] = vdupq_n_f32(0.0f);
    acc.val[1] = vdupq_n_f32(0.0f);

    // The compiler should unroll this
    for (size_t j = 0; j < (NTAPS + 1); j += 4) {
      float32x4x2_t val = vld2q_f32((const float32_t*) &fi[j]);
      acc.val[0] = vmlaq_f32(acc.val[0], val.val[0], taps[j / 4]);
      acc.val[1] = vmlaq_f32(acc.val[1], val.val[1], taps[j / 4]);
    }

    // Sum accumulators
    auto acci = vpadd_f32(vget_low_f32(acc.val[0]), vget_high_f32(acc.val[0]));
    auto accq = vpadd_f32(vget_low_f32(acc.val[1]), vget_high_f32(acc.val[1]));
    acci = vpadd_f32(acci, acci);
    accq = vpadd_f32(accq, accq);
    fo->real(vget_lane_f32(acci, 0));
    fo->imag(vget_lane_f32(accq, 0));

    // Advance input/output cursors
    fi += decimation_;
    fo += 1;
  }

  // Keep final NTAPS samples around
  tmp_.erase(tmp_.begin(), tmp_.end() - NTAPS);
  assert(tmp_.size() == NTAPS);

  // Publish output if applicable
  if (samplePublisher_) {
    samplePublisher_->publish(*output);
  }

  // Return output buffer
  qout->pushWrite(std::move(output));
}
