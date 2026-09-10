#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include "bridge.hpp"

struct Particle {
    int id;
    double x0, y0;   // origin
    double tx, ty;   // target offset
    double start;    // ms timestamp
    double life;     // ms duration
};

// A tiny client-side physics sim: spawns a radial burst of particles at a
// point and eases them outward + up while fading, driven by the C++ main
// loop (one JS DOM write per particle per frame via the bridge).
class ParticleSystem {
public:
    void burst(double cx, double cy, const std::array<std::string, 4>& colors) {
        for (int i = 0; i < 12; i++) {
            double angle = ((double) rand() / RAND_MAX) * 2.0 * M_PI;
            double dist = 50.0 + ((double) rand() / RAND_MAX) * 70.0;
            double size = 5.0 + ((double) rand() / RAND_MAX) * 5.0;
            double life = 450.0 + ((double) rand() / RAND_MAX) * 250.0;

            Particle p;
            p.id = nextId_++;
            p.x0 = cx; p.y0 = cy;
            p.tx = std::cos(angle) * dist;
            p.ty = std::sin(angle) * dist;
            p.start = js_now();
            p.life = life;
            particles_.push_back(p);

            const std::string& color = colors[rand() % colors.size()];
            js_particle_create(p.id, color.c_str(), size);
        }
    }

    void tick() {
        double now = js_now();
        for (size_t i = 0; i < particles_.size();) {
            Particle& p = particles_[i];
            double t = (now - p.start) / p.life;
            if (t >= 1.0) {
                js_particle_remove(p.id);
                particles_.erase(particles_.begin() + i);
                continue;
            }
            double ease = 1.0 - std::pow(1.0 - t, 2.0);
            double x = p.x0 + p.tx * ease;
            double y = p.y0 + p.ty * ease - t * 30.0;
            js_particle_update(p.id, x, y, 1.0 - t);
            i++;
        }
    }

private:
    std::vector<Particle> particles_;
    int nextId_ = 0;
};
