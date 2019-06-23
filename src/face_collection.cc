#include "./face_collection.h"

namespace face_collection {
FaceCollection LoadFaces(FT_Library ft, const vector<string> &face_names) {
  FaceCollection faces;

  for (auto &face_name : face_names) {
    FT_Face face;
    if (FT_New_Face(ft, face_name.c_str(), 0, &face)) {
      fprintf(stderr, "Could not load font\n");
      exit(EXIT_FAILURE);
    }

    if (FT_HAS_COLOR(face)) {
      if (FT_Select_Size(face, 0)) {
        fprintf(stderr, "Could not request the font size (fixed)\n");
        exit(EXIT_FAILURE);
      }
    } else {
      if (FT_Set_Pixel_Sizes(face, kFontPixelWidth, kFontPixelHeight)) {
        fprintf(stderr, "Could not request the font size (in pixels)\n");
        exit(EXIT_FAILURE);
      }
    }

    // The face's size and bbox are populated only after set pixel
    // sizes/select size have been called
    GLsizei width, height;
    if (FT_IS_SCALABLE(face)) {
      width = FT_MulFix(face->bbox.xMax - face->bbox.xMin,
                        face->size->metrics.x_scale) >>
              6;
      height = FT_MulFix(face->bbox.yMax - face->bbox.yMin,
                         face->size->metrics.y_scale) >>
               6;
    } else {
      width = (face->available_sizes[0].width);
      height = (face->available_sizes[0].height);
    }
    faces.push_back(make_tuple(face, width, height));
  }

  return faces;
}

void AssignCodepointsFaces(const string &text, const FaceCollection &faces,
                           CodePointsFacePair *codepoint_faces_pair,
                           hb_buffer_t *buf) {
  const hb_codepoint_t CODEPOINT_MISSING_FACE = UINT32_MAX;
  const hb_codepoint_t CODEPOINT_MISSING = UINT32_MAX;
  // Flag to break the for loop when all of the codepoints have been assigned
  // to a face
  bool all_codepoints_have_a_face = true;
  for (size_t i = 0; (i < faces.size()) && all_codepoints_have_a_face; i++) {
    all_codepoints_have_a_face = false;

    // Reset the buffer, which is reused inbetween iterations
    hb_buffer_clear_contents(buf);

    // Put the text in the buffer
    hb_buffer_add_utf8(buf, text.data(), text.length(), 0, -1);

    // Set the script, language and direction of the buffer
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));

    // Create a font using the face provided by freetype
    FT_Face face = get<0>(faces[i]);
    hb_font_t *font = hb_ft_font_create(face, nullptr);

    vector<hb_feature_t> features(3);
    assert(hb_feature_from_string("kern=1", -1, &features[0]));
    assert(hb_feature_from_string("liga=1", -1, &features[1]));
    assert(hb_feature_from_string("clig=1", -1, &features[2]));

    // Shape the font
    hb_shape(font, buf, &features[0], features.size());

    // Get the glyph and position information
    unsigned int glyph_info_length;
    // unsigned int glyph_position_length;
    hb_glyph_info_t *glyph_info =
        hb_buffer_get_glyph_infos(buf, &glyph_info_length);
    // hb_glyph_position_t *glyph_pos =
    // hb_buffer_get_glyph_positions(buf, &glyph_position_length);

    // assert(glyph_info_length == glyph_position_length);

    // Assign a size to the vector on the first iteration and fill it with
    // UINT32_MAX which represents the absence of a value This assumes that
    // all of the face runs will have the same lengths
    if (i == 0) {
      codepoint_faces_pair->first.resize(glyph_info_length,
                                         CODEPOINT_MISSING_FACE);
      codepoint_faces_pair->second.resize(glyph_info_length, CODEPOINT_MISSING);
    }
    assert(glyph_info_length == codepoint_faces_pair->first.size());

    // Asssign a face to each codepoint if the codepoint hasn't been assigned
    // yet
    for (size_t j = 0; j < glyph_info_length; j++) {
      hb_codepoint_t codepoint = glyph_info[j].codepoint;
      // hb_position_t x_offset = glyph_pos[j].x_offset >> 6;
      // hb_position_t y_offset = glyph_pos[j].y_offset >> 6;
      // hb_position_t x_advance = glyph_pos[j].x_advance >> 6;
      // hb_position_t y_advance = glyph_pos[j].y_advance >> 6;
      // TODO(andrea): use harfbuzz glyph info instead of FreeType's

      if (codepoint != 0 &&
          (codepoint_faces_pair->first)[j] == CODEPOINT_MISSING) {
        codepoint_faces_pair->first[j] = i;
        codepoint_faces_pair->second[j] = codepoint;
      }

      // If we find a glyph which is not present in this face (therefore its
      // codepoint it's 0) and which has not been assigned already then we
      // need to iterate on the next font
      if (codepoint == 0 &&
          (codepoint_faces_pair->second)[j] == CODEPOINT_MISSING) {
        all_codepoints_have_a_face = true;
      }
    }

    // Free the face and the buffer
    hb_font_destroy(font);
  }

  for (size_t i = 0; i < codepoint_faces_pair->first.size(); i++) {
    size_t face = codepoint_faces_pair->first[i];
    hb_codepoint_t codepoint = codepoint_faces_pair->second[i];

    if (face == CODEPOINT_MISSING_FACE && codepoint == CODEPOINT_MISSING) {
      const auto REPLACEMENT_CHARACTER = 0x0000FFFD;
      codepoint_faces_pair->first[i] = 0;
      codepoint_faces_pair->second[i] =
          FT_Get_Char_Index(get<0>(faces[0]), REPLACEMENT_CHARACTER);
    }
  }
}
} // namespace face_collection
