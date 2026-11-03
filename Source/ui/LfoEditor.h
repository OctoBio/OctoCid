#pragma once

#include <JuceHeader.h>
#include "Widgets.h"
#include "../PluginProcessor.h"
#include "../dsp/LfoShape.h"

//==============================================================================
// Éditeur de forme de LFO à la souris, façon Serum :
//  - double-clic dans le vide : ajoute un point ; double-clic sur un point : le supprime
//  - glisser un point : le déplace (aimanté à la grille ; Ctrl : sans aimant ; Maj : fin)
//  - glisser la courbe elle-même (ou sa petite poignée) vers le haut / bas : la bombe ;
//    Alt : toutes les courbes à la fois ; double-clic sur la poignée : courbe droite
//  - glisser dans le vide : sélection rectangle ; on déplace ensuite tous les points choisis
//    ensemble ; Suppr : efface les points choisis ; Ctrl + A : tout choisir
//  - pinceaux : LIBRE (dessin à main levée), MARCHES et RAMPES (comme un step séquenceur) ;
//    Maj + glisser dans le vide : marches, sans changer d'outil
//  - clic droit : formes prêtes, tout effacer, inverser, miroir, copier / coller, annuler
// Le même état (grille, aimant, pinceau, LFO choisi, annulations) est partagé entre
// le petit éditeur du panneau LFO et le grand éditeur du tiroir.
//==============================================================================
struct LfoEditShared
{
    int selected = 0;
    int gridX = 8, gridY = 4;
    bool snap = true;
    int brush = 0;                       // 0 : points, 1 : libre, 2 : marches, 3 : rampes
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
        setWantsKeyboardFocus (true);
        juce::String tip = juce::String::fromUTF8 (
            "Double-clic : ajouter / supprimer un point. Glisser un point (Ctrl : sans aimant, Maj : fin). "
            "Glisser la courbe : la bomber (Alt : toutes). Glisser dans le vide : choisir plusieurs points, Suppr : les effacer. "
            "Maj + glisser : marches. Clic droit : formes, tout effacer, inverser, miroir, copier / coller.");
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
        syncLfo();
        const auto col = octoui::ModHost::lfoColour (lfo());
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff121015).withAlpha (0.88f));
        g.fillRect (r);
        const auto pl = plot();

        // grille
        g.setColour (juce::Colours::white.withAlpha (0.07f));
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
        const bool curveHot = hoverSegment >= 0 || dragMode == DragCurve;
        g.setColour (col.withAlpha (0.25f)); g.strokePath (curve, juce::PathStrokeType (5.0f));
        g.setColour (col);                   g.strokePath (curve, juce::PathStrokeType (1.8f));

        if (isCustom())
        {
            const auto& sh = proc.getLfoShape (lfo());
            // segment survolé / en train d'être courbé : surligné
            if (curveHot)
            {
                const int seg = dragMode == DragCurve ? dragIndex : hoverSegment;
                for (int i = 0; i < sh.count - 1; ++i)
                {
                    if (! (dragMode == DragCurve && curveAll) && i != seg) continue;
                    juce::Path sp;
                    const float x0 = sh.pts[(size_t) i].x, x1 = sh.pts[(size_t) i + 1].x;
                    for (int k = 0; k <= 32; ++k)
                    {
                        const float x = x0 + (x1 - x0) * (float) k / 32.0f;
                        const juce::Point<float> pt (toX (x), toY (sh.eval (std::min (x, x1 - 1.0e-5f))));
                        if (k == 0) sp.startNewSubPath (pt); else sp.lineTo (pt);
                    }
                    g.setColour (juce::Colours::white.withAlpha (0.85f));
                    g.strokePath (sp, juce::PathStrokeType (2.4f));
                }
            }
            // poignées de courbure (milieu de chaque segment)
            for (int i = 0; i < sh.count - 1; ++i)
            {
                const auto& a = sh.pts[(size_t) i]; const auto& b = sh.pts[(size_t) i + 1];
                if (b.x - a.x < 0.012f || std::abs (b.y - a.y) < 0.01f) continue;
                const auto h = handlePos (i);
                const bool hot = hoverHandle == i || hoverSegment == i || (dragMode == DragCurve && (dragIndex == i || curveAll));
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.fillEllipse (juce::Rectangle<float> (hot ? 10.0f : 8.0f, hot ? 10.0f : 8.0f).withCentre (h));
                g.setColour (col.withAlpha (hot ? 1.0f : 0.7f));
                g.drawEllipse (juce::Rectangle<float> (hot ? 8.0f : 6.0f, hot ? 8.0f : 6.0f).withCentre (h), 1.5f);
            }
            // points (choisis : blancs, cerclés)
            for (int i = 0; i < sh.count; ++i)
            {
                const juce::Point<float> pt (toX (sh.pts[(size_t) i].x), toY (sh.pts[(size_t) i].y));
                const bool chosen = isSel (i);
                const bool hot = hoverPoint == i || chosen;
                const float d = hot ? 10.0f : (compact ? 7.0f : 8.5f);
                g.setColour (juce::Colours::black.withAlpha (0.7f));
                g.fillEllipse (juce::Rectangle<float> (d + 3.0f, d + 3.0f).withCentre (pt));
                g.setColour (hot ? juce::Colours::white : col.brighter (0.3f));
                g.fillEllipse (juce::Rectangle<float> (d, d).withCentre (pt));
                if (chosen) { g.setColour (col); g.drawEllipse (juce::Rectangle<float> (d + 5.0f, d + 5.0f).withCentre (pt), 1.4f); }
            }
        }

        // rectangle de sélection
        if (dragMode == RubberBand)
        {
            g.setColour (col.withAlpha (0.15f)); g.fillRect (rubber);
            g.setColour (col.withAlpha (0.8f));  g.drawRect (rubber, 1.0f);
        }

        // tête de lecture : sortie réelle du LFO
        const float ph = proc.lfoPhaseView[(size_t) lfo()].load (std::memory_order_relaxed);
        const float v = proc.lfoValueView[(size_t) lfo()].load (std::memory_order_relaxed);
        const float vy = unipolar() ? pl.getBottom() - v * pl.getHeight() : pl.getCentreY() - v * pl.getHeight() * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawVerticalLine ((int) toX (ph), pl.getY(), pl.getBottom());
        g.setColour (juce::Colours::white);
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ toX (ph), vy }));

        // valeur du point déplacé
        if (dragMode == DragPoint && isCustom() && dragIndex >= 0 && dragIndex < proc.getLfoShape (lfo()).count)
        {
            const auto& p = proc.getLfoShape (lfo()).pts[(size_t) dragIndex];
            const float val = unipolar() ? p.y * 100.0f : (p.y * 2.0f - 1.0f) * 100.0f;
            juce::String txt = juce::String (p.x * (float) shared.gridX, 2) + "/" + juce::String (shared.gridX) + "  " + juce::String (juce::roundToInt (val)) + " %";
            auto tr = juce::Rectangle<float> (92.0f, 15.0f).withCentre ({ toX (p.x), toY (p.y) - 14.0f })
                          .constrainedWithin (getLocalBounds().toFloat().reduced (2.0f));
            g.setColour (juce::Colours::black.withAlpha (0.75f)); g.fillRoundedRectangle (tr, 3.0f);
            g.setColour (juce::Colours::white); g.setFont (OctoLook::monoFont (10.0f));
            g.drawText (txt, tr, juce::Justification::centred);
        }

        // en-tête
        g.setFont (OctoLook::monoFont (compact ? 10.0f : 11.0f));
        g.setColour (col);
        g.drawText (unipolar() ? "+" : juce::String::fromUTF8 ("±"), polArea(), juce::Justification::centred);
        if (compact)
        {
            g.setColour (col.withAlpha (0.85f));
            g.drawText (juce::String::fromUTF8 ("✥ LFO ") + juce::String (lfo() + 1), gripArea(), juce::Justification::centredLeft);
        }
        if (! compact)
        {
            static const char* tools[4] = { "POINTS : double-clic ajoute / supprime, glisser la courbe la bombe", "PINCEAU LIBRE : dessine à main levée",
                                            "PINCEAU : MARCHES", "PINCEAU : RAMPES" };
            g.setColour (col.withAlpha (0.7f));
            g.drawText (juce::String::fromUTF8 (tools[juce::jlimit (0, 3, shared.brush)]), r.removeFromTop (15.0f).reduced (8.0f, 0).withTrimmedRight (26.0f),
                        juce::Justification::centredLeft);
        }
        g.setColour (col.withAlpha (0.6f));
        g.drawRect (getLocalBounds().toFloat(), 1.2f);
    }

    //--------------------------------------------------------------------------
    void mouseMove (const juce::MouseEvent& e) override
    {
        syncLfo();
        const int hp = isCustom() ? pointAt (e.position) : -1;
        const int hh = (isCustom() && hp < 0) ? handleAt (e.position) : -1;
        const int hs = (isCustom() && hp < 0 && hh < 0 && shared.brush == 0) ? segmentNear (e.position) : -1;
        if (hp != hoverPoint || hh != hoverHandle || hs != hoverSegment) { hoverPoint = hp; hoverHandle = hh; hoverSegment = hs; repaint(); }
        setMouseCursor (compact && gripArea().contains (e.position) ? juce::MouseCursor::DraggingHandCursor
                        : hp >= 0 ? juce::MouseCursor::PointingHandCursor
                        : (hh >= 0 || hs >= 0) ? juce::MouseCursor::UpDownResizeCursor
                        : juce::MouseCursor::CrosshairCursor);
    }
    void mouseExit (const juce::MouseEvent&) override { hoverPoint = hoverHandle = hoverSegment = -1; repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        syncLfo();
        grabKeyboardFocus();
        dragMode = None; dragIndex = -1; dndStarted = false;
        if (e.mods.isPopupMenu()) { const int p = pointAt (e.position); if (p >= 0 && ! isSel (p)) selectOnly (p); showMenu (p); return; }
        if (polArea().contains (e.position)) { togglePolarity(); return; }
        if (compact && gripArea().contains (e.position)) { dragMode = Grip; return; }

        before = currentShape();
        const int brush = shared.brush;
        const int p = isCustom() ? pointAt (e.position) : -1;

        if (brush == 1) { startFreeDraw (e.position); return; }
        if (brush >= 2 || (e.mods.isShiftDown() && p < 0 && handleAt (e.position) < 0)) { startPaint (e.position, brush == 3); return; }

        if (p >= 0)
        {
            if (e.mods.isCommandDown() || e.mods.isCtrlDown()) { toggleSel (p); repaint(); }
            else if (! isSel (p)) selectOnly (p);
            dragMode = DragPoint; dragIndex = p;
            const auto& sh = proc.getLfoShape (lfo());
            for (int i = 0; i < sh.count; ++i) startPts[(size_t) i] = sh.pts[(size_t) i];
            return;
        }
        ensureCustom();
        int seg = handleAt (e.position);
        if (seg < 0) seg = segmentNear (e.position);
        if (seg < 0 && e.mods.isAltDown()) seg = segmentAtX (e.position.x);
        if (seg >= 0)
        {
            dragMode = DragCurve; dragIndex = seg; curveAll = e.mods.isAltDown();
            const auto& sh = proc.getLfoShape (lfo());
            for (int i = 0; i < sh.count; ++i) startPts[(size_t) i] = sh.pts[(size_t) i];
            return;
        }
        // dans le vide : sélection rectangle
        dragMode = RubberBand;
        rubberStart = e.position;
        rubber = {};
        if (! (e.mods.isCommandDown() || e.mods.isCtrlDown())) clearSel();
        repaint();
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
        if (dragMode == Paint) { paintAt (e.position); return; }
        if (dragMode == FreeDraw) { freeDrawAt (e.position); return; }
        if (dragMode == RubberBand)
        {
            rubber = juce::Rectangle<float> (rubberStart, e.position);
            const auto& sh = proc.getLfoShape (lfo());
            if (! (e.mods.isCommandDown() || e.mods.isCtrlDown())) clearSel();
            for (int i = 0; i < sh.count; ++i)
                if (rubber.contains (juce::Point<float> (toX (sh.pts[(size_t) i].x), toY (sh.pts[(size_t) i].y)))) sel[(size_t) i] = true;
            repaint();
            return;
        }

        auto sh = proc.getLfoShape (lfo());
        const bool snap = shared.snap != (e.mods.isCtrlDown() || e.mods.isCommandDown());
        const bool fine = e.mods.isShiftDown();
        const auto pl = plot();

        if (dragMode == DragPoint && dragIndex >= 0 && dragIndex < sh.count)
        {
            float dx = (float) e.getDistanceFromDragStartX() / pl.getWidth() * (fine ? 0.2f : 1.0f);
            float dy = -(float) e.getDistanceFromDragStartY() / pl.getHeight() * (fine ? 0.2f : 1.0f);
            const auto& a = startPts[(size_t) dragIndex];
            if (snap && ! fine)   // c'est le point attrapé qui s'aimante, les autres le suivent
            {
                dx = std::round ((a.x + dx) * (float) shared.gridX) / (float) shared.gridX - a.x;
                dy = std::round ((a.y + dy) * (float) shared.gridY) / (float) shared.gridY - a.y;
            }
            // les points choisis bougent ensemble, sans jamais passer par-dessus un voisin non choisi
            float dxMin = -1.0f, dxMax = 1.0f;
            for (int i = 0; i < sh.count; ++i)
            {
                if (! isSel (i) || i == 0 || i == sh.count - 1) continue;
                int l = i - 1; while (l > 0 && isSel (l)) --l;
                int rr = i + 1; while (rr < sh.count - 1 && isSel (rr)) ++rr;
                dxMin = std::max (dxMin, startPts[(size_t) l].x - startPts[(size_t) i].x);
                dxMax = std::min (dxMax, startPts[(size_t) rr].x - startPts[(size_t) i].x);
            }
            dx = juce::jlimit (std::min (dxMin, 0.0f), std::max (dxMax, 0.0f), dx);
            for (int i = 0; i < sh.count; ++i)
            {
                if (! isSel (i)) continue;
                auto& pt = sh.pts[(size_t) i];
                pt.y = juce::jlimit (0.0f, 1.0f, startPts[(size_t) i].y + dy);
                if (i > 0 && i < sh.count - 1) pt.x = juce::jlimit (0.0f, 1.0f, startPts[(size_t) i].x + dx);
            }
            apply (sh, false);
        }
        else if (dragMode == DragCurve && dragIndex >= 0 && dragIndex < sh.count - 1)
        {
            // vers le haut : la courbe se bombe vers le haut (quel que soit le sens du segment)
            const float d = (float) -e.getDistanceFromDragStartY() / (fine ? 400.0f : 80.0f);
            for (int i = 0; i < sh.count - 1; ++i)
                if (curveAll || i == dragIndex)
                    sh.pts[(size_t) i].curve = juce::jlimit (-1.0f, 1.0f, startPts[(size_t) i].curve + d);
            apply (sh, false);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragMode == DragPoint || dragMode == DragCurve || dragMode == Paint || dragMode == FreeDraw)
            if (before.toString() != proc.getLfoShape (lfo()).toString())
                pushUndo (before);
        if (dragMode == Paint || dragMode == FreeDraw) clearSel();
        dragMode = None; dragIndex = -1;
        repaint();
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (polArea().contains (e.position) || (compact && gripArea().contains (e.position)) || shared.brush != 0) return;
        ensureCustom();
        auto sh = proc.getLfoShape (lfo());
        const auto prev = sh;
        const int p = pointAt (e.position);
        if (p >= 0) { sh.remove (p); clearSel(); }
        else if (const int h = handleAt (e.position); h >= 0) sh.pts[(size_t) h].curve = 0.0f;
        else
        {
            const auto pl = plot();
            float x = (e.position.x - pl.getX()) / pl.getWidth();
            float y = (pl.getBottom() - e.position.y) / pl.getHeight();
            if (shared.snap != (e.mods.isCtrlDown() || e.mods.isCommandDown()))
            {
                x = std::round (x * (float) shared.gridX) / (float) shared.gridX;
                y = std::round (y * (float) shared.gridY) / (float) shared.gridY;
            }
            const int at = sh.add (juce::jlimit (0.001f, 0.999f, x), juce::jlimit (0.0f, 1.0f, y));
            if (at >= 0) { // le nouveau point garde la courbure du segment coupé
                if (at > 0) sh.pts[(size_t) at].curve = sh.pts[(size_t) at - 1].curve;
                selectOnly (at);
            }
        }
        pushUndo (prev);
        apply (sh, false);
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        const bool cmd = k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown();
        if (k.isKeyCode (juce::KeyPress::deleteKey) || k.isKeyCode (juce::KeyPress::backspaceKey)) { deleteSelected(); return true; }
        if (cmd && (k.getKeyCode() == 'A' || k.getKeyCode() == 'a')) { selectAll(); return true; }
        if (cmd && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { undo(); return true; }
        if (cmd && (k.getKeyCode() == 'C' || k.getKeyCode() == 'c')) { copy(); return true; }
        if (cmd && (k.getKeyCode() == 'V' || k.getKeyCode() == 'v')) { paste(); return true; }
        if (k.isKeyCode (juce::KeyPress::escapeKey)) { clearSel(); repaint(); return true; }
        return false;
    }

    //--------------------------------------------------------------------------
    // Actions (aussi utilisées par la barre d'outils du grand éditeur)
    void loadPreset (int i)
    {
        pushUndo (currentShape());
        clearSel();
        apply (octocid::LfoShape::preset (i), true);
    }
    void flip()   { ensureCustom(); auto s = proc.getLfoShape (lfo()); pushUndo (s); s.flipVertical(); apply (s, false); }
    void mirror() { ensureCustom(); auto s = proc.getLfoShape (lfo()); pushUndo (s); s.mirror(); clearSel(); apply (s, false); }
    void clearAll()   // tous les points supprimés : ligne plate au milieu
    {
        pushUndo (currentShape());
        clearSel();
        apply (octocid::LfoShape::fromList ({ { 0, 0.5f }, { 1, 0.5f } }), true);
    }
    void deleteSelected()
    {
        if (! isCustom()) return;
        auto s = proc.getLfoShape (lfo());
        const auto prev = s;
        for (int i = s.count - 2; i >= 1; --i) if (isSel (i)) s.remove (i);
        clearSel();
        if (s.toString() == prev.toString()) return;
        pushUndo (prev);
        apply (s, false);
    }
    void selectAll()
    {
        ensureCustom();
        for (int i = 0; i < proc.getLfoShape (lfo()).count; ++i) sel[(size_t) i] = true;
        repaint();
    }
    void undo()
    {
        auto& u = shared.undo[(size_t) lfo()];
        if (u.empty()) return;
        auto s = u.back(); u.pop_back();
        clearSel();
        apply (s, true);
    }
    void copy() { shared.clipboard = currentShape(); shared.hasClipboard = true; }
    void paste() { if (shared.hasClipboard) { pushUndo (currentShape()); clearSel(); apply (shared.clipboard, true); } }

    void showPresetMenu (juce::Component* target)
    {
        juce::PopupMenu m;
        for (int i = 0; i < octocid::LfoShape::numPresets; ++i) m.addItem (1 + i, juce::String::fromUTF8 (octocid::LfoShape::presetName (i)));
        juce::Component::SafePointer<LfoEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [safe] (int r) { if (safe != nullptr && r > 0) safe->loadPreset (r - 1); });
    }

private:
    enum DragMode { None, DragPoint, DragCurve, Paint, FreeDraw, RubberBand, Grip };

    juce::Rectangle<float> plot() const
    {
        auto r = getLocalBounds().toFloat();
        return compact ? r.reduced (7.0f, 0.0f).withTrimmedTop (16.0f).withTrimmedBottom (7.0f)
                       : r.reduced (10.0f, 0.0f).withTrimmedTop (17.0f).withTrimmedBottom (8.0f);
    }
    juce::Rectangle<float> polArea() const  { return getLocalBounds().toFloat().removeFromTop (16.0f).removeFromRight (22.0f); }
    juce::Rectangle<float> gripArea() const { return getLocalBounds().toFloat().removeFromTop (16.0f).withWidth (64.0f).translated (4.0f, 0.0f); }
    float toX (float x) const { const auto p = plot(); return p.getX() + x * p.getWidth(); }
    float toY (float y) const { const auto p = plot(); return p.getBottom() - y * p.getHeight(); }

    // changement d'onglet : la sélection ne vaut que pour le LFO où elle a été faite
    void syncLfo() { if (lastLfo != lfo()) { lastLfo = lfo(); clearSel(); } }
    bool isSel (int i) const { return i >= 0 && i < octocid::LfoShape::maxPoints && sel[(size_t) i]; }
    void clearSel() { sel.fill (false); }
    void selectOnly (int i) { clearSel(); if (i >= 0 && i < octocid::LfoShape::maxPoints) sel[(size_t) i] = true; repaint(); }
    void toggleSel (int i) { if (i >= 0 && i < octocid::LfoShape::maxPoints) sel[(size_t) i] = ! sel[(size_t) i]; }

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
        int best = -1; float bd = 9.0f;
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
            if (sh.pts[(size_t) i + 1].x - sh.pts[(size_t) i].x >= 0.012f && m.getDistanceFrom (handlePos (i)) < 8.0f) return i;
        return -1;
    }
    int segmentAtX (float px) const
    {
        const auto& sh = proc.getLfoShape (lfo());
        const float x = (px - plot().getX()) / plot().getWidth();
        for (int i = 0; i < sh.count - 1; ++i)
            if (x >= sh.pts[(size_t) i].x && x <= sh.pts[(size_t) i + 1].x && sh.pts[(size_t) i + 1].x > sh.pts[(size_t) i].x) return i;
        return -1;
    }
    // segment dont la courbe passe près de la souris (on attrape la ligne elle-même)
    int segmentNear (juce::Point<float> m) const
    {
        if (! isCustom()) return -1;
        const int seg = segmentAtX (m.x);
        if (seg < 0) return -1;
        const float x = (m.x - plot().getX()) / plot().getWidth();
        return std::abs (toY (proc.getLfoShape (lfo()).eval (x)) - m.y) < 7.0f ? seg : -1;
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
        if (u.size() > 60) u.erase (u.begin());
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

    // ---- pinceau marches / rampes
    void startPaint (juce::Point<float> m, bool ramps)
    {
        dragMode = Paint; paintRamps = ramps;
        ensureCustom();
        cells.assign ((size_t) shared.gridX, 0.0f);
        for (int i = 0; i < shared.gridX; ++i)
            cells[(size_t) i] = valueAt (((float) i + 0.5f) / (float) shared.gridX);
        lastCell = -1;
        paintAt (m);
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
        s.paintSteps (n, cells, paintRamps);
        apply (s, true);
    }

    // ---- pinceau libre : on dessine la courbe, elle est simplifiée en points
    static constexpr int kSamples = 256;
    void startFreeDraw (juce::Point<float> m)
    {
        dragMode = FreeDraw;
        ensureCustom();
        samples.resize (kSamples + 1);
        for (int i = 0; i <= kSamples; ++i) samples[(size_t) i] = valueAt ((float) i / (float) kSamples);
        lastSample = -1;
        freeDrawAt (m);
    }
    void freeDrawAt (juce::Point<float> m)
    {
        const auto pl = plot();
        const int idx = juce::jlimit (0, kSamples, juce::roundToInt ((m.x - pl.getX()) / pl.getWidth() * (float) kSamples));
        const float y = juce::jlimit (0.0f, 1.0f, (pl.getBottom() - m.y) / pl.getHeight());
        if (lastSample < 0) samples[(size_t) idx] = y;
        else
        {
            const int a = std::min (lastSample, idx), b = std::max (lastSample, idx);
            const float ya = lastSample <= idx ? lastY : y, yb = lastSample <= idx ? y : lastY;
            for (int i = a; i <= b; ++i) samples[(size_t) i] = b == a ? y : ya + (yb - ya) * (float) (i - a) / (float) (b - a);
        }
        lastSample = idx; lastY = y;
        apply (simplify(), true);
    }
    // Ramer-Douglas-Peucker : garde les points utiles, au plus 64
    octocid::LfoShape simplify() const
    {
        std::vector<bool> keep ((size_t) kSamples + 1, false);
        keep[0] = keep[(size_t) kSamples] = true;
        float eps = 0.006f;
        for (int pass = 0; pass < 6; ++pass)
        {
            std::fill (keep.begin(), keep.end(), false);
            keep[0] = keep[(size_t) kSamples] = true;
            std::vector<std::pair<int, int>> stack { { 0, kSamples } };
            while (! stack.empty())
            {
                auto [a, b] = stack.back(); stack.pop_back();
                float worst = 0.0f; int wi = -1;
                for (int i = a + 1; i < b; ++i)
                {
                    const float t = (float) (i - a) / (float) (b - a);
                    const float d = std::abs (samples[(size_t) i] - (samples[(size_t) a] + (samples[(size_t) b] - samples[(size_t) a]) * t));
                    if (d > worst) { worst = d; wi = i; }
                }
                if (wi >= 0 && worst > eps) { keep[(size_t) wi] = true; stack.push_back ({ a, wi }); stack.push_back ({ wi, b }); }
            }
            int n = 0; for (bool k : keep) n += k ? 1 : 0;
            if (n <= octocid::LfoShape::maxPoints) break;
            eps *= 1.8f;
        }
        octocid::LfoShape s; s.count = 0;
        for (int i = 0; i <= kSamples && s.count < octocid::LfoShape::maxPoints; ++i)
            if (keep[(size_t) i]) s.pts[(size_t) s.count++] = { (float) i / (float) kSamples, samples[(size_t) i], 0.0f };
        s.sortAndClamp();
        return s;
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
        int nSel = 0;
        if (isCustom()) for (int i = 1; i < proc.getLfoShape (lfo()).count - 1; ++i) nSel += isSel (i) ? 1 : 0;
        if (point >= 0)
        {
            const auto& sh = proc.getLfoShape (lfo());
            m.addItem (2, "Courbe droite", isCustom() && point < sh.count - 1);
        }
        m.addItem (1, nSel > 1 ? juce::String::fromUTF8 ("Supprimer les ") + juce::String (nSel) + juce::String::fromUTF8 (" points choisis")
                               : juce::String ("Supprimer le point"), nSel > 0);
        m.addItem (9, juce::String::fromUTF8 ("Tout sélectionner (Ctrl + A)"));
        m.addItem (5, juce::String::fromUTF8 ("Tout effacer (ligne plate)"));
        m.addSeparator();
        m.addSubMenu (juce::String::fromUTF8 ("Formes prêtes"), presets);
        m.addItem (3, "Inverser (haut / bas)");
        m.addItem (4, "Miroir (gauche / droite)");
        m.addSeparator();
        m.addItem (6, "Copier la forme");
        m.addItem (7, "Coller la forme", shared.hasClipboard);
        m.addItem (8, "Annuler (Ctrl + Z)", ! shared.undo[(size_t) lfo()].empty());
        juce::Component::SafePointer<LfoEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, point] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            auto& ed = *safe;
            if (r >= 100) { ed.loadPreset (r - 100); return; }
            switch (r)
            {
                case 1: ed.deleteSelected(); return;
                case 3: ed.flip(); return;
                case 4: ed.mirror(); return;
                case 5: ed.clearAll(); return;
                case 6: ed.copy(); return;
                case 7: ed.paste(); return;
                case 8: ed.undo(); return;
                case 9: ed.selectAll(); return;
                case 2:
                {
                    ed.ensureCustom();
                    auto s = ed.proc.getLfoShape (ed.lfo());
                    if (point < 0 || point >= s.count) return;
                    ed.pushUndo (s);
                    s.pts[(size_t) point].curve = 0.0f;
                    ed.apply (s, true);
                    return;
                }
                default: return;
            }
        });
    }

    OctoCidAudioProcessor& proc;
    LfoEditShared& shared;
    const bool compact;
    DragMode dragMode = None;
    int dragIndex = -1, hoverPoint = -1, hoverHandle = -1, hoverSegment = -1, lastCell = -1, lastLfo = -1, lastSample = -1;
    bool dndStarted = false, curveAll = false, paintRamps = false;
    float lastY = 0.0f;
    std::array<bool, octocid::LfoShape::maxPoints> sel {};
    std::array<octocid::LfoShape::Point, octocid::LfoShape::maxPoints> startPts {};
    juce::Point<float> rubberStart;
    juce::Rectangle<float> rubber;
    octocid::LfoShape before;
    std::vector<float> cells, samples;
};
