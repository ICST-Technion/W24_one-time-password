import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';

class StatisticsPage extends StatelessWidget {
  final List<List<int>> statistics;
  final List<String> daysOfWeek = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];

  StatisticsPage({required this.statistics});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Number of entries'),
      ),
      body: ListView.builder(
        itemCount: statistics.length,
        itemBuilder: (context, index) {
          return Column(
            children: [
              SizedBox(height: 20),
              Text(
                daysOfWeek[index], // Display the day of the week instead of "Day ${index + 1}"
                style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
              ),
              SizedBox(height: 10),
              AspectRatio(
                aspectRatio: 1.7,
                child: Card(
                  elevation: 0,
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(4)),
                  color: const Color(0xff2c4260),
                  child: BarChart(
                    BarChartData(
                      alignment: BarChartAlignment.spaceAround,
                      maxY: 28, // Maximum value for each diagram
                      minY: 0, // Adjusted minimum value to ensure zero line alignment
                      groupsSpace: 16,
                      barTouchData: BarTouchData(
                        enabled: false,
                        touchTooltipData: BarTouchTooltipData(
                          tooltipBgColor: Colors.transparent,
                          tooltipPadding: const EdgeInsets.all(0),
                          tooltipMargin: 8,
                          getTooltipItem: (
                              BarChartGroupData group,
                              int groupIndex,
                              BarChartRodData rod,
                              int rodIndex,
                              ) {
                            return BarTooltipItem(
                              rod.y.round().toString(),
                              TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
                            );
                          },
                        ),
                      ),
                      titlesData: FlTitlesData(
                        show: true,
                        bottomTitles: SideTitles(
                          showTitles: true,
                          getTextStyles: (value) => const TextStyle(
                            color: Color(0xff7589a2),
                            fontWeight: FontWeight.bold,
                            fontSize: 14,
                          ),
                          margin: 20,
                          getTitles: (double value) {
                            // Display specific hour labels
                            switch (value.toInt()) {
                              case 0:
                                return '0:00';
                              case 6:
                                return '6:00';
                              case 12:
                                return '12:00';
                              case 18:
                                return '18:00';
                              default:
                                return '';
                            }
                          },
                        ),
                        leftTitles: SideTitles(
                          showTitles: true,
                          getTextStyles: (value) => const TextStyle(
                            color: Color(0xff7589a2),
                            fontWeight: FontWeight.bold,
                            fontSize: 14,
                          ),
                          margin: 32,
                          reservedSize: 14,
                          interval: 5, // Set interval to 5 for resolution of 5
                          getTitles: (value) {
                            // Return an empty string for negative values
                            if (value < 0) {
                              return '';
                            }
                            return value.toInt().toString();
                          },
                        ),
                      ),
                      gridData: FlGridData(
                        show: true,
                        drawHorizontalLine: true, // Keep horizontal lines visible
                        checkToShowHorizontalLine: (value) => value % 5 == 0, // Show horizontal lines for values that are multiples of 5
                      ),
                      borderData: FlBorderData(
                        show: false,
                      ),
                      barGroups: List.generate(
                        statistics[index].length,
                            (hourIndex) => BarChartGroupData(
                          x: hourIndex,
                          barRods: [
                            BarChartRodData(y: statistics[index][hourIndex].toDouble(), colors: [Colors.lightBlueAccent, Colors.greenAccent])
                          ],
                          showingTooltipIndicators: [0],
                        ),
                      ),
                    ),
                  ),
                ),
              ),
            ],
          );
        },
      ),
    );
  }
}
