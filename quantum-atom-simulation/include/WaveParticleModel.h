#ifndef WAVE_PARTICLE_MODEL_H
#define WAVE_PARTICLE_MODEL_H

#include "AtomModel.h"

class WaveParticleModel : public AtomModel {
private:
    double waveAmplitude;
    double waveFrequency;
    double waveSpeed;
    double particlePosition[3];
    double particleVelocity[3];
    bool showWave;
    bool showParticle;
    
public:
    WaveParticleModel(int atomicNumber = 1);
    
    void update(double dt) override;
    void render() override;
    void reset() override;
    
    void setShowWave(bool show);
    void setShowParticle(bool show);
    
private:
    double calculateWaveFunction(double x, double y, double z, double t) const;
};

#endif // WAVE_PARTICLE_MODEL_H
