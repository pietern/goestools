These files are the GeoJSON equivalents from the files available from
Natural Earth. Originals from https://www.naturalearthdata.com/.

The files here are medium scale (1:50m). I think it's a good trade off
between file size and visual appeal. While the large scale data is
finer grained, the medium scale data is sufficient for the products
transmitter over LRIT/HRIT.

To generate these files, install Node.js, and subsequently the
`shapefile` package (https://www.npmjs.com/package/shapefile). This
package ships with the `shp2json` tool that converts a Shapefile (SHP)
to the GeoJSON format.

Using `ogr2ogr` instead of `shp2json' will allow conversion of 
a Shapefile (SHP) from a points based geometry like most GIS
systems, to a polygon based geometry shape that is processed
by goestools with this command:

```
ogr2ogr -f "GeoJSON" -dialect SQLite -sql "select ST_Buffer(geometry, 0.01) from inputfile" output.json inputfile.shp
```

See https://gdal.org/ for installation instructions for `ogr2ogr`.
