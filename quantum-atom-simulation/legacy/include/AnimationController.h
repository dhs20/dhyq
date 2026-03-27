#ifndef ANIMATION_CONTROLLER_H
#define ANIMATION_CONTROLLER_H

class AnimationController {
private:
    double time;
    double deltaTime;
    bool isPlaying;
    double speed;
    
public:
    AnimationController();
    
    void update(double dt);
    void play();
    void pause();
    void reset();
    
    bool getIsPlaying() const;
    double getTime() const;
    double getDeltaTime() const;
    double getSpeed() const;
    
    void setSpeed(double speed);
};

#endif // ANIMATION_CONTROLLER_H
