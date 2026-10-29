#pragma once

namespace octocid
{
    //==========================================================================
    // Pile des notes tenues (taille fixe, aucune allocation) pour le jeu mono
    // legato : la dernière note enfoncée est prioritaire.
    //==========================================================================
    class NoteStack
    {
    public:
        static constexpr int kMax = 16;

        void clear() noexcept { count = 0; }
        bool empty() const noexcept { return count == 0; }
        int size() const noexcept { return count; }
        int top() const noexcept { return count > 0 ? notes[count - 1] : -1; }

        void push (int note) noexcept
        {
            remove (note);
            if (count == kMax)              // pile pleine : on oublie la plus ancienne
            {
                for (int i = 1; i < kMax; ++i) notes[i - 1] = notes[i];
                --count;
            }
            notes[count++] = note;
        }

        void remove (int note) noexcept
        {
            for (int i = 0; i < count; ++i)
            {
                if (notes[i] == note)
                {
                    for (int j = i + 1; j < count; ++j) notes[j - 1] = notes[j];
                    --count;
                    return;
                }
            }
        }

    private:
        int notes[kMax] {};
        int count = 0;
    };
}
