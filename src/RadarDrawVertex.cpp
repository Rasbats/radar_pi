/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#include "RadarDrawVertex.h"

PLUGIN_BEGIN_NAMESPACE

bool RadarDrawVertex::Init(int newColorOption) {
  if (newColorOption) {
    // Don't care in vertex mode
  }

  /* for (size_t i = 0; i < LINES_PER_ROTATION; i++) {
     spokes[i].n = 0;
   }*/

  wxLogMessage(wxT("BR24radar_pi: CPU oriented OpenGL vertex draw loaded"));
  return true;
}

#define ADD_VERTEX_POINT(angle, radius, r, g, b, a)          \
  {                                                          \
    line->points[count].x = m_polarLookup->x[angle][radius]; \
    line->points[count].y = m_polarLookup->y[angle][radius]; \
    line->points[count].red = r;                             \
    line->points[count].green = g;                           \
    line->points[count].blue = b;                            \
    line->points[count].alpha = a;                           \
    count++;                                                 \
  }

void RadarDrawVertex::SetBlob(VertexLine* line, int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green,
                              GLubyte blue, GLubyte alpha) {
  if (r2 == 0) {
    return;
  }
  int arc1 = MOD_ROTATION2048(angle_begin);
  int arc2 = MOD_ROTATION2048(angle_end);
  size_t count = line->count;

  if (line->count + VERTEX_PER_QUAD > line->allocated) {
    const size_t extra = 8 * VERTEX_PER_QUAD;
    line->points = (VertexPoint*)realloc(line->points, (line->allocated + extra) * sizeof(VertexPoint));
    if (!line->points) {
      if (!m_oom) {
        wxLogError(wxT("BR24radar_pi: Out of memory"));
        m_oom = true;
      }
      return;
    }
    line->allocated += extra;
    m_count += extra;
    if (m_pi->m_settings.verbose >= 2) {
      wxLogMessage(wxT("BR24radar_pi: increased vertex array allocation to %u points, %u bytes"), line->allocated,
                   m_count * sizeof(VertexPoint));
    }
  }

  // First triangle
  ADD_VERTEX_POINT(arc1, r1, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha);

  // Second triangle

  ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha);
  ADD_VERTEX_POINT(arc2, r2, red, green, blue, alpha);

  line->count = count;
}

void RadarDrawVertex::ProcessRadarSpoke(SpokeBearing angle, UINT8* data, size_t len) {
  GLubyte red, green, blue;
  GLubyte alpha = 255 * (MAX_OVERLAY_TRANSPARENCY - m_pi->m_settings.overlay_transparency) / MAX_OVERLAY_TRANSPARENCY;
  BlobColor previous_color = BLOB_NONE;
  GLubyte strength = 0;
  time_t now = time(0);

  wxMutexLocker lock(m_mutex);

  int r_begin = 0;
  int r_end = 0;

  VertexLine* line = &m_vertices[angle];

  if (!line->points) {
    static size_t INITIAL_ALLOCATION = 600;  // Empirically found to be enough for a complicated picture
    line->allocated = INITIAL_ALLOCATION * VERTEX_PER_QUAD;
    m_count += INITIAL_ALLOCATION * VERTEX_PER_QUAD;
    line->points = (VertexPoint*)malloc(line->allocated * sizeof(VertexPoint));
    if (!line->points) {
      wxLogError(wxT("BR24radar_pi: Out of memory"));
      line->allocated = 0;
      line->count = 0;
      return;
    }
  }
  line->count = 0;
  line->timeout = now + m_pi->m_settings.max_age;

  for (size_t radius = 0; radius < len; radius++) {
    strength = data[radius];
    BlobColor actual_color = m_pi->m_color_map[strength];

    if (actual_color == previous_color) {
      // continue with same color, just register it
      r_end++;
    } else if (previous_color == BLOB_NONE && actual_color != BLOB_NONE) {
      // blob starts, no display, just register
      r_begin = radius;
      r_end = r_begin + 1;
      previous_color = actual_color;  // new color
    } else if (previous_color != BLOB_NONE && (previous_color != actual_color)) {
      red = m_pi->m_color_map_red[previous_color];
      green = m_pi->m_color_map_green[previous_color];
      blue = m_pi->m_color_map_blue[previous_color];

      SetBlob(line, angle, angle + 1, r_begin, r_end, red, green, blue, alpha);

      previous_color = actual_color;
      if (actual_color != BLOB_NONE) {  // change of color, start new blob
        r_begin = radius;
        r_end = r_begin + 1;
      }
    }
  }

  if (previous_color != BLOB_NONE) {  // Draw final blob
    red = m_pi->m_color_map_red[previous_color];
    green = m_pi->m_color_map_green[previous_color];
    blue = m_pi->m_color_map_blue[previous_color];

    SetBlob(line, angle, angle + 1, r_begin, r_end, red, green, blue, alpha);
  }
}

void RadarDrawVertex::DrawRadarImage(wxPoint center, double scale, double rotation) {
  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glPushMatrix();
  glTranslated(center.x, center.y, 0);
  if (rotation != 0.0) {
    glRotated(rotation, 0.0, 0.0, 1.0);
  }
  glScaled(scale, scale, 1.);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  time_t now = time(0);
  {
    wxMutexLocker lock(m_mutex);

    for (size_t i = 0; i < LINES_PER_ROTATION; i++) {
      VertexLine* line = &m_vertices[i];
      if (TIMED_OUT(now, line->timeout)) {
        continue;
      }

      glVertexPointer(2, GL_FLOAT, sizeof(VertexPoint), &line->points[0].x);
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexPoint), &line->points[0].red);
      glDrawArrays(GL_TRIANGLES, 0, line->count);
    }
  }
  glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
  glDisableClientState(GL_COLOR_ARRAY);

  glPopMatrix();  // Undo translated/scaled
  glPopAttrib();  // Undo blend
}

PLUGIN_END_NAMESPACE