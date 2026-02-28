#pragma once

namespace Framework {
    class GameObjectComposition;

    struct BehaviourFCT
    {
        using InitFn = void(*)(GameObjectComposition*);
        using UpdateFn = void(*)(GameObjectComposition*, float);
        using EndFn = void(*)(GameObjectComposition*);

        InitFn Init{ nullptr };
        UpdateFn Update{ nullptr };
        EndFn End{ nullptr };
    };
}
