WRAP_CLASS("itk::PolygonGroupSpatialObject" POINTER)
  foreach(d ${WRAP_ITK_DIMS})
    WRAP_TEMPLATE(${d} ${d})
  endforeach(d)
END_WRAP_CLASS()