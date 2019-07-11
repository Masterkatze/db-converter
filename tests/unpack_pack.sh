rm -r /tmp/db_converter_test
mkdir /tmp/db_converter_test -p
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --unpack ~/.local/share/GSC/SCOP/resources/resources.db0 --out /tmp/db_converter_test/original_unpacked --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --pack /tmp/db_converter_test/original_unpacked/ --out /tmp/db_converter_test/1.db --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --unpack /tmp/db_converter_test/1.db --out /tmp/db_converter_test/1_unpacked --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --pack /tmp/db_converter_test/1_unpacked/ --out /tmp/db_converter_test/2.db --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --unpack /tmp/db_converter_test/2.db --out /tmp/db_converter_test/2_unpacked --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --pack /tmp/db_converter_test/2_unpacked/ --out /tmp/db_converter_test/3.db --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --unpack /tmp/db_converter_test/3.db --out /tmp/db_converter_test/3_unpacked --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --pack /tmp/db_converter_test/3_unpacked/ --out /tmp/db_converter_test/4.db --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --unpack /tmp/db_converter_test/4.db --out /tmp/db_converter_test/4_unpacked --xdb
~/qtp/build-db_converter-Desktop-Debug/src/db_converter --pack /tmp/db_converter_test/4_unpacked/ --out /tmp/db_converter_test/5.db --xdb
ls -l /tmp/db_converter_test/1.db /tmp/db_converter_test/2.db /tmp/db_converter_test/3.db /tmp/db_converter_test/4.db /tmp/db_converter_test/5.db
