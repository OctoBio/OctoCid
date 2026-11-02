#pragma once

#include <JuceHeader.h>
#include "Widgets.h"
#include "../PluginProcessor.h"
#include "../dsp/LfoShape.h"

//==============================================================================
// Éditeur de forme de LFO à la souris (inspiré de Serum, Vital, Kilohearts) :
//  - double-clic dans le vide : ajoute un point ; double-clic sur un point : le supprime
//  - glisser un point : le déplace (aimanté à la grille ; Ctrl : sans aimant ; Maj : fin)
//  - glisser un segment (ou sa poignée) vers le haut / bas : le courbe ; double-clic poignée : droit
//  - pinceau Marches / Rampes : on peint les cases de la grille comme un step séquenceur
//  - clic droit : formes prêtes, inverser, miroir, copier / coller, annuler
// Le même état (grille, aimant, pinceau, LFO choisi, annulations) est partagé entre
// le petit éditeur du panneau LFO et le grand éditeur du tiroir.
//==============================================================================
struct LfoEditShared
{
    int selected = 0;
    int gridX = 8, gridY = 4;
    bool snap = true;
    int brush = 0;                       // 0 : points, 1 : marches, 2 : rampes
    std::array<std::vector<octocid::LfoShape>, ModParams::numLfos> undo;
    octocid::LfoShape clipboard;
    bool hasClipboard = false;
    juce::Array<juce::Component*> views;
    std::vector<std::function<void (int)>> selectListeners;   // changement de LFO (onglets)

    void repaintAll() { for (auto* v : views) v->repaint(); }
    void select (int i) { selected = juce::jlimit (0, ModParams::numLfos - 1, i); for (auto& f : selectListeners) f (selected); repaintAll(); }

    static constexpr int gridXChoices[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 };
    static constexpr int gridYChoices[] = { 1, 2, 3, 4, 6, 8, 12, 16 };
};

class LfoEditor : public juce::Component, public juce::SettableTooltipClient
{
public:
    LfoEditor (OctoCidAudioProcessor& p, LfoEditShared& s, bool isCompact)
        : proc (p), shared (s), compact (isCompact)
    {
        shared.views.add (this);
        juce::String tip = juce::String::fromUTF8 (
            "Double-clic : ajouter / supprimer un point. Glisser un point : le déplacer (Ctrl : sans aimant, Maj : fin). "
            "Glisser une courbe : la bomber. Clic droit : formes prêtes, inverser, miroir, copier / coller, annuler.");
        if (compact) tip << juce::String::fromUTF8 (" Attraper « ✥ LFO n » (en haut à gauche) et le lâcher sur un potard : modulation.");
        setTooltip (tip);
    }
    ~LfoEditor() override { shared.views.removeFirstMatchingValue (this); }

    int lfo() const noexcept { return shared.selected; }

    // ---- état courant
    int shapeIndex() const { return (int) proc.apvts.getRawParameterValue (ModParams::lfoId (lfo(), "shape"))->load(); }
    bool isCustom() const { return shapeIndex() == octocid::ModLfo::Custom; }
    bool unipolar() const { return proc.apvts.getRawParameterValue (ModParams::lfoId (lfo(), "pol"))->load() > 0.5f; }

    // valeur 0..1 affichée (forme dessinée ou forme calculée)
    float valueAt (float x) const
    {
        if (isCustom()) return proc.getLfoShape (lfo()).eval (x);
        static const float rnd[8] = { 0.65f, 0.1f, 0.95f, 0.4f, 0.8f, 0.2f, 0.55f, 0.03f };
        switch (shapeIndex())
        {
            case 1: return x < 0.5f ? 2.0f * x : 2.0f - 2.0f * x;
            case 2: return x;
            case 3: return 1.0f - x;
            case 4: return x < 0.5f ? 1.0f : 0.0f;
            case 5: return rnd[(int) (x * 8.0f) & 7];
            case 6: { const float p = x * 8.0f; const int i = (int) p; const float t = 0.5f - 0.5f * std::cos (3.14159265f * (p - (float) i));
                      return rnd[i & 7] + (rnd[(i + 1) & 7] - rnd[i & 7]) * t; }
            default: return 0.5f + 0.5f * std::sin (6.28318531f * x);
        }
    }

    //--------------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        const auto col = octoui::ModHost::lfoColour (lfo());
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff121015).withAlpha (0.88f));
        g.fillRect (r);
        const auto pl = plot();

        // grille
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        for (int i = 1; i < shared.gridX; ++i) g.drawVerticalLine ((int) (pl.getX() + pl.getWidth() * (float) i / (float) shared.gridX), pl.getY(), pl.getBottom());
        for (int i = 1; i < shared.gridY; ++i) g.drawHorizontalLine ((int) (pl.getY() + pl.getHeight() * (float) i / (float) shared.gridY), pl.getX(), pl.getRight());
        g.setColour (col.withAlpha (0.18f));
        g.drawHorizontalLine ((int) (unipolar() ? pl.getBottom() : pl.getCentreY()), pl.getX(), pl.getRight());

        // courbe
        juce::Path curve, fill;
        const int steps = juce::jmax (64, (int) pl.getWidth());
        for (int k = 0; k <= steps; ++k)
        {
            const float x = (float) k / (float) steps;
            const juce::Point<float> pt (toX (x), toY (valueAt (x)));
            if (k == 0) curve.startNewSubPath (pt); else curve.lineTo (pt);
        }
        fill = curve;
        const float baseY = unipolar() ? pl.getBottom() : pl.getCentreY();
        fill.lineTo (pl.getRight(), baseY); fill.lineTo (pl.getX(), baseY); fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.28f), 0, pl.getY(), col.withAlpha (0.04f), 0, pl.getBottom(), false));
        g.fillPath (fill);
        g.setColour (col.withAlpha (0.25f)); g.strokePath (curve, juce::PathStrokeType (5.0f));
        g.setColour (col);                   g.strokePath (curve, juce::PathStrokeType (1.8f));

        // points et poignées de courbure (forme dessinée)
        if (isCustom())
        {
            const auto& sh = proc.getLfoShape (lfo());
            for (int i = 0; i < sh.count - 1; ++i)
            {
                const auto& a = sh.pts[(size_t) i]; const auto& b = sh.pts[(size_t) i + 1];
                if (b.x - a.x < 0.012f || std::abs (b.y - a.y) < 0.01f) continue;
                const auto h = handlePos (i);
                const bool hot = hoverHandle == i || (dragMode == DragCurve && dragIndex == i);
                g.setColour (col.withAlpha (hot ? 1.0f : 0.55f));
                g.drawEllipse (juce::Rectangle<float> (hot ? 8.0f : 6.0f, hot ? 8.0f : 6.0f).withCentre (h), 1.3f);
            }
            for (int i = 0; i < sh.count; ++i)
            {
                const juce::Point<float> pt (toX (sh.pts[(size_t) i].x), toY (sh.pts[(size_t) i].y));
                const bool hot = hoverPoint == i || (dragMode == DragPoint && dragIndex == i);
                const float d = hot ? 10.0f : (compact ? 6.5f : 8.0f);
                g.setColour (juce::Colours::black.withAlpha (0.7f));
                g.fillEllipse (juce::Rectangle<float> (d + 3.0f, d + 3.0f).withCentre (pt));
                g.setColour (hot ? juce::Colours::white : col.brighter (0.3f));
                g.fillEllipse (juce::Rectangle<float> (d, d).withCentre (pt));
            }
        }

        // tête de lecture : sortie réelle du LFO
        const float ph = proc.lfoPhaseView[(size_t) lfo()].load (std::memory_order_relaxed);
        const float v = proc.lfoValueView[(size_t) lfo()].load (std::memory_order_relaxed);
        const float vy = unipolar() ? pl.getBottom() - v * pl.getHeight() : pl.getCentreY() - v * pl.getHeight() * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawVerticalLine ((int) toX (ph), pl.getY(), pl.getBottom());
        g.setColour (juce::Colours::white);
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ toX (ph), vy }));

        // en-tête
        g.setFont (OctoLook::monoFont (compact ? 10.0f : 11.0f));
        g.setColour (col);
        g.drawText (unipolar() ? "+" : juce::String::fromUTF8 ("±"), polArea(), juce::Justification::centred);
        if (compact)
        {
            g.setColour (col.withAlpha (0.85f));
            g.drawText (juce::String::fromUTF8 ("✥ LFO ") + juce::String (lfo() + 1), gripArea(), juce::Justification::centredLeft);
        }
        if (shared.brush > 0 && ! compact)
        {
            g.setColour (col.withAlpha (0.8f));
            g.drawText (shared.brush == 1 ? "PINCEAU : MARCHES" : "PINCEAU : RAMPES", r.removeFromTop (14.0f).reduced (6.0f, 0), juce::Justification::centredLeft);
        }
        g.setColour (col.withAlpha (0.6f));
        g.drawRect (getLocalBounds().toFloat(), 1.2f);
    }

    //--------------------------------------------------------------------------
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int hp = isCustom() ? pointAt (e.position) : -1;
        const int hh = (isCustom() && hp < 0) ? handleAt (e.position) : -1;
        if (hp != hoverPoint || hh != hoverHandle) { hoverPoint = hp; hoverHandle = hh; repaint(); }
        setMouseCursor (gripArea().contains (e.position) && compact ? juce::MouseCursor::DraggingHandCursor
                        : hp >= 0 ? juce::MouseCursor::PointingHandCursor
                        : juce::MouseCursor::CrosshairCursor);
    }
    void mouseExit (const juce::MouseEvent&) override { hoverPoint = hoverHandle = -1; repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragMode = None; dragIndex = -1; dndStarted = false;
        if (e.mods.isPopupMenu()) { showMenu (pointAt (e.position)); return; }
        if (polArea().contains (e.position)) { togglePolarity(); return; }
        if (compact && gripArea().contains (e.position)) { dragMode = Grip; return; }

        before = currentShape();
        if (shared.brush > 0)
        {
            dragMode = Paint;
            ensureCustom();
            cells.assign ((size_t) shared.gridX, 0.0f);
            for (int i = 0; i < shared.gridX; ++i)
                cells[(size_t) i] = valueAt (((float) i + 0.5f) / (float) shared.gridX);
            lastCell = -1;
            paintAt (e.position);
            return;
        }
        const int p = isCustom() ? pointAt (e.position) : -1;
        if (p >= 0) { dragMode = DragPoint; dragIndex = p; startPt = proc.getLfoShape (lfo()).pts[(size_t) p]; return; }
        ensureCustom();
        const int h = handleAt (e.position);
        const int seg = h >= 0 ? h : segmentAt (e.position.x);
        if (seg >= 0) { dragMode = DragCurve; dragIndex = seg; startCurve = proc.getLfoShape (lfo()).pts[(size_t) seg].curve; }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragMode == Grip)
        {
            if (! dndStarted && e.getDistanceFromDragStart() > 4)
                if (auto* dc = juce::DragAndDropContainer::findParentDragContainerFor (this))
                {
                    dndStarted = true;
                    dc->startDragging ("octolfo:" + juce::String (lfo()), this, juce::ScaledImage (dragImage()), true);
                }
            return;
        }
        auto sh = proc.getLfoShape (lfo());
        const bool snap = shared.snap != (e.mods.isCtrlDown() || e.mods.isCommandDown());
        const bool fine = e.mods.isShiftDown();
        if (dragMode == Paint) { paintAt (e.position); return; }
        if (dragMode == DragPoint && dragIndex >= 0 && dragIndex < sh.count)
        {
            const auto pl = plot();
            float x = startPt.x + (float) e.getDistanceFromDragStartX() / pl.getWidth() * (fine ? 0.2f : 1.0f);
            float y = startPt.y - (float) e.getDistanceFromDragStartY() / pl.getHeight() * (fine ? 0.2f : 1.0f);
            if (snap && ! fine)
            {
                x = std::round (x * (float) shared.gridX) / (float) shared.gridX;
                y = std::round (y * (float) shared.gridY) / (float) shared.gridY;
            }
            auto& pt = sh.pts[(size_t) dragIndex];
            if (dragIndex == 0) x = 0.0f;
            else if (dragIndex == sh.count - 1) x = 1.0f;
            else x = juce::jlimit (sh.pts[(size_t) dragIndex - 1].x, sh.pts[(size_t) dragIndex + 1].x, x);
            pt.x = x; pt.y = juce::jlimit (0.0f, 1.0f, y);
            apply (sh, false);
        }
        else if (dragMode == DragCurve && dragIndex >= 0 && dragIndex < sh.count - 1)
        {
            const float d = (float) -e.getDistanceFromDragStartY() / (fine ? 400.0f : 90.0f);
            sh.pts[(size_t) dragIndex].curve = juce::jlimit (-1.0f, 1.0f, startCurve + d);
            apply (sh, false);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragMode == DragPoint || dragMode == DragCurve || dragMode == Paint)
            if (before.toString() != proc.getLfoShape (lfo()).toString())
                pushUndo (before);
        dragMode = None; dragIndex = -1;
        repaint();
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (polArea().contains (e.position) || (compact && gripArea().contains (e.position))) return;
        ensureCustom();
        auto sh = proc.getLfoShape (lfo());
        const auto prev = sh;
        const int p = pointAt (e.position);
        if (p >= 0) sh.remove (p);
        else if (const int h = handleAt (e.position); h >= 0) sh.pts[(size_t) h].curve = 0.0f;
        else
        {
            const auto pl = plot();
            float x = (e.position.x - pl.getX()) / pl.getWidth();
            float y = (pl.getBottom() - e.position.y) / pl.getHeight();
            if (shared.snap != e.mods.isCtrlDown())
            {
                x = std::round (x * (float) shared.gridX) / (float) shared.gridX;
                y = std::round (y * (float) shared.gridY) / (float) shared.gridY;
            }
            sh.add (juce::jlimit (0.001f, 0.999f, x), juce::jlimit (0.0f, 1.0f, y));
        }
        pushUndo (prev);
        apply (sh, false);
    }

    //--------------------------------------------------------------------------
    // Actions (aussi utilisées par la barre d'outils du grand éditeur)
    void loadPreset (int i)
    {
        pushUndo (currentShape());
        apply (octocid::LfoShape::preset (i), true);
    }
    void flip()   { ensureCustom(); auto s = proc.getLfoShape (lfo()); pushUndo (s); s.flipVertical(); apply (s, false); }
    void mirror() { ensureCustom(); auto s = proc.getLfoShape (lfo()); pushUndo (s); s.mirror(); apply (s, false); }
    void undo()
    {
        auto& u = shared.undo[(size_t) lfo()];
        if (u.empty()) return;
        auto s = u.back(); u.pop_back();
        apply (s, true);
    }
    void copy() { shared.clipboard = currentShape(); shared.hasClipboard = true; }
    void paste() { if (shared.hasClipboard) { pushUndo (currentShape()); apply (shared.clipboard, true); } }

    void showPresetMenu (juce::Component* target)
    {
        juce::PopupMenu m;
        for (int i = 0; i < octocid::LfoShape::numPresets; ++i) m.addItem (1 + i, juce::String::fromUTF8 (octocid::LfoShape::presetName (i)));
        juce::Component::SafePointer<LfoEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [safe] (int r) { if (safe != nullptr && r > 0) safe->loadPreset (r - 1); });
    }

private:
    enum DragMode { None, DragPoint, DragCurve, Paint, Grip };

    juce::Rectangle<float> plot() const
    {
        auto r = getLocalBounds().toFloat();
        return compact ? r.reduced (7.0f, 0.0f).withTrimmedTop (16.0f).withTrimmedBottom (7.0f)
                       : r.reduced (10.0f, 0.0f).withTrimmedTop (16.0f).withTrimmedBottom (8.0f);
    }
    juce::Rectangle<float> polArea() const  { return getLocalBounds().toFloat().removeFromTop (16.0f).removeFromRight (22.0f); }
    juce::Rectangle<float> gripArea() const { return getLocalBounds().toFloat().removeFromTop (16.0f).withWidth (64.0f).translated (4.0f, 0.0f); }
    float toX (float x) const { const auto p = plot(); return p.getX() + x * p.getWidth(); }
    float toY (float y) const { const auto p = plot(); return p.getBottom() - y * p.getHeight(); }

    juce::Point<float> handlePos (int seg) const
    {
        const auto& sh = proc.getLfoShape (lfo());
        const float xm = 0.5f * (sh.pts[(size_t) seg].x + sh.pts[(size_t) seg + 1].x);
        return { toX (xm), toY (sh.eval (xm)) };
    }
    int pointAt (juce::Point<float> m) const
    {
        if (! isCustom()) return -1;
        const auto& sh = proc.getLfoShape (lfo());
        int best = -1; float bd = 8.0f;
        for (int i = 0; i < sh.count; ++i)
        {
            const float d = m.getDistanceFrom ({ toX (sh.pts[(size_t) i].x), toY (sh.pts[(size_t) i].y) });
            if (d < bd) { bd = d; best = i; }
        }
        return best;
    }
    int handleAt (juce::Point<float> m) const
    {
        if (! isCustom()) return -1;
        const auto& sh = proc.getLfoShape (lfo());
        for (int i = 0; i < sh.count - 1; ++i)
            if (sh.pts[(size_t) i + 1].x - sh.pts[(size_t) i].x >= 0.012f && m.getDistanceFrom (handlePos (i)) < 7.0f) return i;
        return -1;
    }
    int segmentAt (float px) const
    {
        const auto& sh = proc.getLfoShape (lfo());
        const float x = (px - plot().getX()) / plot().getWidth();
        for (int i = 0; i < sh.count - 1; ++i)
            if (x >= sh.pts[(size_t) i].x && x <= sh.pts[(size_t) i + 1].x && sh.pts[(size_t) i + 1].x > sh.pts[(size_t) i].x) return i;
        return -1;
    }

    octocid::LfoShape currentShape() const
    {
        if (isCustom()) return proc.getLfoShape (lfo());
        const int s = shapeIndex();
        return octocid::LfoShape::preset (s <= 4 ? s : 0);
    }

    // une forme calculée devient une forme dessinée équivalente dès qu'on la touche
    void ensureCustom()
    {
        if (isCustom()) return;
        const auto s = currentShape();
        pushUndo (proc.getLfoShape (lfo()));
        proc.setLfoShape (lfo(), s);
        setShapeParam (octocid::ModLfo::Custom);
    }
    void setShapeParam (int index)
    {
        if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (ModParams::lfoId (lfo(), "shape"))))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) index));
            prm->endChangeGesture();
        }
    }
    void apply (const octocid::LfoShape& s, bool forceCustom)
    {
        proc.setLfoShape (lfo(), s);
        if (forceCustom || ! isCustom()) setShapeParam (octocid::ModLfo::Custom);
        shared.repaintAll();
    }
    void pushUndo (const octocid::LfoShape& s)
    {
        auto& u = shared.undo[(size_t) lfo()];
        u.push_back (s);
        if (u.size() > 40) u.erase (u.begin());
    }
    void togglePolarity()
    {
        if (auto* prm = proc.apvts.getParameter (ModParams::lfoId (lfo(), "pol")))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
            prm->endChangeGesture();
        }
        shared.repaintAll();
    }

    void paintAt (juce::Point<float> m)
    {
        const auto pl = plot();
        const int n = shared.gridX;
        const int cell = juce::jlimit (0, n - 1, (int) ((m.x - pl.getX()) / pl.getWidth() * (float) n));
        float y = juce::jlimit (0.0f, 1.0f, (pl.getBottom() - m.y) / pl.getHeight());
        if (shared.snap) y = std::round (y * (float) shared.gridY) / (float) shared.gridY;
        const int a = lastCell < 0 ? cell : std::min (lastCell, cell), b = lastCell < 0 ? cell : std::max (lastCell, cell);
        for (int c = a; c <= b; ++c) cells[(size_t) c] = y;
        lastCell = cell;
        auto s = proc.getLfoShape (lfo());
        s.paintSteps (n, cells, shared.brush == 2);
        apply (s, true);
    }

    juce::Image dragImage() const
    {
        juce::Image img (juce::Image::ARGB, 64, 22, true);
        juce::Graphics g (img);
        g.setColour (octoui::ModHost::lfoColour (lfo())); g.fillRoundedRectangle (0, 0, 64, 22, 4);
        g.setColour (OctoColours::ink); g.setFont (OctoLook::monoFont (12.0f));
        g.drawText ("LFO " + juce::String (lfo() + 1), 0, 0, 64, 22, juce::Justification::centred);
        return img;
    }

    void showMenu (int point)
    {
        juce::PopupMenu m, presets;
        for (int i = 0; i < octocid::LfoShape::numPresets; ++i) presets.addItem (100 + i, juce::String::fromUTF8 (octocid::LfoShape::presetName (i)));
        if (point >= 0)
        {
            const auto& sh = proc.getLfoShape (lfo());
            m.addItem (1, "Supprimer le point", point > 0 && point < sh.count - 1);
            m.addItem (2, "Courbe droite", point < sh.count - 1);
            m.addSeparator();
        }
        m.addSubMenu (juce::String::fromUTF8 ("Formes prêtes"), presets);
        m.addItem (3, "Inverser (haut / bas)");
        m.addItem (4, "Miroir (gauche / droite)");
        m.addItem (5, "Ligne plate");
        m.addSeparator();
        m.addItem (6, "Copier la forme");
        m.addItem (7, "Coller la forme", shared.hasClipboard);
        m.addItem (8, "Annuler", ! shared.undo[(size_t) lfo()].empty());
        juce::Component::SafePointer<LfoEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, point] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            auto& ed = *safe;
            if (r >= 100) { ed.loadPreset (r - 100); return; }
            if (r == 3) { ed.flip(); return; }
            if (r == 4) { ed.mirror(); return; }
            if (r == 6) { ed.copy(); return; }
            if (r == 7) { ed.paste(); return; }
            if (r == 8) { ed.undo(); return; }
            ed.ensureCustom();
            auto s = ed.proc.getLfoShape (ed.lfo());
            ed.pushUndo (s);
            if (r == 1) s.remove (point);
            else if (r == 2) s.pts[(size_t) point].curve = 0.0f;
            else if (r == 5) s = octocid::LfoShape::fromList ({ { 0, 0.5f }, { 1, 0.5f } });
            ed.apply (s, true);
        });
    }

    OctoCidAudioProcessor& proc;
    LfoEditShared& shared;
    const bool compact;
    DragMode dragMode = None;
    int dragIndex = -1, hoverPoint = -1, hoverHandle = -1, lastCell = -1;
    bool dndStarted = false;
    octocid::LfoShape::Point startPt;
    float startCurve = 0.0f;
    octocid::LfoShape before;
    std::vector<float> cells;
};
