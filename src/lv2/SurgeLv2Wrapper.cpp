#include "SurgeLv2Wrapper.h"
#include "SurgeLv2Util.h"
#include "SurgeLv2Ui.h" // for editor's setParameterAutomated
#include <cstring>

///
SurgeLv2Wrapper::SurgeLv2Wrapper(double sampleRate)
    : fSynthesizer(new SurgeSynthesizer(this)),
      fDataLocation(new void *[NumPorts]()),
      fOldControlValues(new float[n_total_params]()),
      fSampleRate(sampleRate)
{
    // needed?
    fSynthesizer->time_data.ppqPos = 0;
}

SurgeLv2Wrapper::~SurgeLv2Wrapper()
{
}

void SurgeLv2Wrapper::updateDisplay()
{
    // should I do anything particular here?
}

void SurgeLv2Wrapper::setParameterAutomated(int externalparam, float value)
{
    #warning LV2 setParameterAutomated might arrive from MIDI
    fEditor->setParameterAutomated(externalparam, value);
}

LV2_Handle SurgeLv2Wrapper::instantiate(const LV2_Descriptor *descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
{
    std::unique_ptr<SurgeLv2Wrapper> self(new SurgeLv2Wrapper(sample_rate));

    SurgeSynthesizer *synth = self->fSynthesizer.get();
    synth->setSamplerate(sample_rate);

    auto *featureUridMap = (const LV2_URID_Map *)SurgeLv2::requireFeature(LV2_URID__map, features);

    self->fUridMidiEvent = featureUridMap->map(featureUridMap->handle, LV2_MIDI__MidiEvent);
    self->fUridAtomBlank = featureUridMap->map(featureUridMap->handle, LV2_ATOM__Blank);
    self->fUridAtomObject = featureUridMap->map(featureUridMap->handle, LV2_ATOM__Object);
    self->fUridAtomDouble = featureUridMap->map(featureUridMap->handle, LV2_ATOM__Double);
    self->fUridAtomFloat = featureUridMap->map(featureUridMap->handle, LV2_ATOM__Float);
    self->fUridAtomInt = featureUridMap->map(featureUridMap->handle, LV2_ATOM__Int);
    self->fUridAtomLong = featureUridMap->map(featureUridMap->handle, LV2_ATOM__Long);
    self->fUridTimePosition = featureUridMap->map(featureUridMap->handle, LV2_TIME__Position);
    self->fUridTime_beatsPerMinute = featureUridMap->map(featureUridMap->handle, LV2_TIME__beatsPerMinute);
    self->fUridTime_speed = featureUridMap->map(featureUridMap->handle, LV2_TIME__speed);
    self->fUridTime_beat = featureUridMap->map(featureUridMap->handle, LV2_TIME__beat);

    return (LV2_Handle)self.release();
}

void SurgeLv2Wrapper::connectPort(LV2_Handle instance, uint32_t port, void *data_location)
{
    SurgeLv2Wrapper *self = (SurgeLv2Wrapper *)instance;
    self->fDataLocation[port] = data_location;
}

void SurgeLv2Wrapper::activate(LV2_Handle instance)
{
    SurgeLv2Wrapper *self = (SurgeLv2Wrapper *)instance;
    SurgeSynthesizer *s = self->fSynthesizer.get();

    self->fBlockPos = 0;

    for (unsigned pNth = 0; pNth < n_total_params; ++pNth)
    {
        unsigned index = s->remapExternalApiToInternalId(pNth);
        self->fOldControlValues[pNth] = s->getParameter01(index);
    }

    s->audio_processing_active = true;
}

void SurgeLv2Wrapper::run(LV2_Handle instance, uint32_t sample_count)
{
    SurgeLv2Wrapper *self = (SurgeLv2Wrapper *)instance;
    SurgeSynthesizer *s = self->fSynthesizer.get();
    double sampleRate = self->fSampleRate;

    self->fFpuState.set();

    for (unsigned pNth = 0; pNth < n_total_params; ++pNth)
    {
        float portValue = *(float *)(self->fDataLocation[pNth]);
        if (portValue != self->fOldControlValues[pNth])
        {
            unsigned index = s->remapExternalApiToInternalId(pNth);
            s->setParameter01(index, portValue);
            self->fOldControlValues[pNth] = portValue;
        }
    }

    s->process_input = false/*(!plug_is_synth || input_connected)*/;

    bool isPlaying = SurgeLv2::isNotZero(self->fTimePositionSpeed);
    bool hasValidTempo = SurgeLv2::isGreaterThanZero(self->fTimePositionTempoBpm);
    bool hasValidPosition = self->fTimePositionBeat >= 0.0;

    if (hasValidTempo)
    {
        if (isPlaying)
            s->time_data.tempo = self->fTimePositionTempoBpm * std::fabs(self->fTimePositionSpeed);
    }
    else
    {
        s->time_data.tempo = 120.0;
    }

    if (isPlaying && hasValidPosition)
        s->time_data.ppqPos = self->fTimePositionBeat; // TODO is it correct?

    auto *eventSequence = (const LV2_Atom_Sequence *)self->fDataLocation[pEvents];
    const LV2_Atom_Event *eventIter = lv2_atom_sequence_begin(&eventSequence->body);
    const LV2_Atom_Event *event = SurgeLv2::popNextEvent(eventSequence, &eventIter);

    const float *inputs[NumInputs];
    float *outputs[NumOutputs];

    for (uint32_t i = 0; i < NumInputs; ++i)
        inputs[i] = (const float *)self->fDataLocation[pAudioInput1 + i];
    for (uint32_t i = 0; i < NumOutputs; ++i)
        outputs[i] = (float *)self->fDataLocation[pAudioOutput1 + i];

    for (uint32_t i = 0; i < sample_count; ++i)
    {
      if (self->fBlockPos == 0)
      {
         // move clock
         s->time_data.ppqPos +=
             (double)BLOCK_SIZE * s->time_data.tempo / (60. * sampleRate);

         // process events for the current block
         while (event && i >= event->time.frames)
         {
             self->handleEvent(event);
             event = SurgeLv2::popNextEvent(eventSequence, &eventIter);
         }

         // run sampler engine for the current block
         s->process();
      }

      if (s->process_input)
      {
         int inp;
         for (inp = 0; inp < NumInputs; inp++)
         {
            s->input[inp][self->fBlockPos] = inputs[inp][i];
         }
      }

      int outp;
      for (outp = 0; outp < NumOutputs; outp++)
      {
          outputs[outp][i] = (float)s->output[outp][self->fBlockPos]; // replacing
      }

      self->fBlockPos++;
      if (self->fBlockPos >= BLOCK_SIZE)
         self->fBlockPos = 0;
    }

    while (event)
    {
        self->handleEvent(event);
        event = SurgeLv2::popNextEvent(eventSequence, &eventIter);
    }

   self->fFpuState.restore();
}

void SurgeLv2Wrapper::handleEvent(const LV2_Atom_Event *event)
{
   if (event->body.type == fUridMidiEvent)
   {
      const char *midiData = (char *)event + sizeof(*event);
      int status = midiData[0] & 0xf0;
      int channel = midiData[0] & 0x0f;
      if (status == 0x90 || status == 0x80) // we only look at notes
      {
         int newnote = midiData[1] & 0x7f;
         int velo = midiData[2] & 0x7f;
         if ((status == 0x80) || (velo == 0))
         {
            fSynthesizer->releaseNote((char)channel, (char)newnote, (char)velo);
         }
         else
         {
            fSynthesizer->playNote((char)channel, (char)newnote, (char)velo, 0/*event->detune*/);
         }
      }
      else if (status == 0xe0) // pitch bend
      {
         long value = (midiData[1] & 0x7f) + ((midiData[2] & 0x7f) << 7);
         fSynthesizer->pitchBend((char)channel, value - 8192);
      }
      else if (status == 0xB0) // controller
      {
         if (midiData[1] == 0x7b || midiData[1] == 0x7e)
            fSynthesizer->allNotesOff(); // all notes off
         else
            fSynthesizer->channelController((char)channel, midiData[1] & 0x7f, midiData[2] & 0x7f);
      }
      else if (status == 0xC0) // program change
      {
         fSynthesizer->programChange((char)channel, midiData[1] & 0x7f);
      }
      else if (status == 0xD0) // channel aftertouch
      {
         fSynthesizer->channelAftertouch((char)channel, midiData[1] & 0x7f);
      }
      else if (status == 0xA0) // poly aftertouch
      {
         fSynthesizer->polyAftertouch((char)channel, midiData[1] & 0x7f, midiData[2] & 0x7f);
      }
      else if ((status == 0xfc) || (status == 0xff)) //  MIDI STOP or reset
      {
         fSynthesizer->allNotesOff();
      }
   }
   else if ((event->body.type == fUridAtomBlank || event->body.type == fUridAtomObject) &&
            (((const LV2_Atom_Object*)&event->body)->body.otype) == fUridTimePosition)
   {
       const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&event->body;

       LV2_Atom *beatsPerMinute = nullptr;
       LV2_Atom *speed = nullptr;
       LV2_Atom *beat = nullptr;

       lv2_atom_object_get(
           obj,
           fUridTime_beatsPerMinute, &beatsPerMinute,
           fUridTime_speed, &speed,
           fUridTime_beat, &beat,
           0);

       auto atomGetNumber = [this](const LV2_Atom *atom, double *number) -> bool {
           if (atom->type == fUridAtomDouble)
               *number = ((const LV2_Atom_Double *)atom)->body;
           else if (atom->type == fUridAtomFloat)
               *number = ((const LV2_Atom_Float *)atom)->body;
           else if (atom->type == fUridAtomInt)
               *number = ((const LV2_Atom_Int *)atom)->body;
           else if (atom->type == fUridAtomLong)
               *number = ((const LV2_Atom_Long *)atom)->body;
           else
               return false;
           return true;
       };

       if (speed)
           atomGetNumber(beatsPerMinute, &fTimePositionSpeed);
       if (beatsPerMinute)
           atomGetNumber(beatsPerMinute, &fTimePositionTempoBpm);
       if (beat)
           atomGetNumber(beat, &fTimePositionBeat);
   }
}

void SurgeLv2Wrapper::deactivate(LV2_Handle instance)
{
    SurgeLv2Wrapper *self = (SurgeLv2Wrapper *)instance;

    self->fSynthesizer->allNotesOff();
    self->fSynthesizer->audio_processing_active = false;
}

void SurgeLv2Wrapper::cleanup(LV2_Handle instance)
{
    SurgeLv2Wrapper *self = (SurgeLv2Wrapper *)instance;
    delete self;
}

const void *SurgeLv2Wrapper::extensionData(const char *uri)
{
    return nullptr;
}

LV2_Descriptor SurgeLv2Wrapper::createDescriptor()
{
    LV2_Descriptor desc = {};
    desc.URI = "https://github.com/linuxmao-org/surge";
    desc.instantiate = &instantiate;
    desc.connect_port = &connectPort;
    desc.activate = &activate;
    desc.run = &run;
    desc.deactivate = &deactivate;
    desc.cleanup = &cleanup;
    desc.extension_data = &extensionData;
    return desc;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    if (index == 0)
    {
        static LV2_Descriptor desc = SurgeLv2Wrapper::createDescriptor();
        return &desc;
    }
    return nullptr;
}
