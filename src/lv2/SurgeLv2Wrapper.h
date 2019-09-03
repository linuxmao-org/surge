#pragma once
#include "SurgeSynthesizer.h"
#include "AllLv2.h"
#include "util/FpuState.h"
#include <memory>

class SurgeLv2Ui;

class SurgeLv2Wrapper
{
public:
   static LV2_Descriptor createDescriptor();

   explicit SurgeLv2Wrapper(double sampleRate);
   ~SurgeLv2Wrapper();

   void setEditor(SurgeLv2Ui* editor)
   {
      fEditor = editor;
   }

   enum
   {
      NumInputs = 2,
      NumOutputs = 2,
   };

   enum
   {
      EventBufferSize = 8192,
   };

   enum
   {
      /* [0,n] parameters */
      pEvents = n_total_params,
      pAudioInput1,
      pAudioOutput1 = pAudioInput1 + NumInputs,
      NumPorts = pAudioOutput1 + NumOutputs,
   };

   SurgeSynthesizer* synthesizer() const noexcept
   {
      return fSynthesizer.get();
   }

   // PluginLayer
   void updateDisplay();
   void setParameterAutomated(int externalparam, float value);

private:
   static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                                 double sample_rate,
                                 const char* bundle_path,
                                 const LV2_Feature* const* features);
   static void connectPort(LV2_Handle instance, uint32_t port, void* data_location);
   static void activate(LV2_Handle instance);
   static void run(LV2_Handle instance, uint32_t sample_count);
   void handleEvent(const LV2_Atom_Event* event);
   static void deactivate(LV2_Handle instance);
   static void cleanup(LV2_Handle instance);
   static const void* extensionData(const char* uri);

private:
   std::unique_ptr<SurgeSynthesizer> fSynthesizer;
   std::unique_ptr<void*[]> fDataLocation;
   std::unique_ptr<float[]> fOldControlValues;
   double fSampleRate = 44100.0;

   uint32_t fBlockPos = 0;
   FpuState fFpuState;

   double fTimePositionSpeed = 0.0;
   double fTimePositionTempoBpm = 0.0;
   double fTimePositionBeat = 0.0;

   LV2_URID fUridMidiEvent;
   LV2_URID fUridAtomBlank;
   LV2_URID fUridAtomObject;
   LV2_URID fUridAtomDouble;
   LV2_URID fUridAtomFloat;
   LV2_URID fUridAtomInt;
   LV2_URID fUridAtomLong;
   LV2_URID fUridTimePosition;
   LV2_URID fUridTime_beatsPerMinute;
   LV2_URID fUridTime_speed;
   LV2_URID fUridTime_beat;

   SurgeLv2Ui* fEditor = nullptr;
};
