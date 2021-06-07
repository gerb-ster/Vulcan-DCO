<?php

$arraySize = 1024;

/**
 * Calculate all frequencies for every step of the ADC
 */

// Calculation = (2 ^ (2 * voltage)) * 27.5

$frequencyMap = [];
$voltsPerADCStep = 8 / $arraySize;

for($i = 0; $i < $arraySize; $i++)
{
    $frequencyMap[] = pow(2, ($i*$voltsPerADCStep)) * 27.5;   
}

/**
 * Calculate all ocr_step_values for every step of the ADC, based on the frequencies
 */

// (F_CPU / current_prescaler) / (frequency * 2)

$ocrStepMap = [];
//$ocrStepValue = 20000000 / 8

foreach($frequencyMap as $frequencyMapValue)
{
    $ocrStepMap[] = round(2500000 / ($frequencyMapValue * 2));
}

// do some fancy formatting
$sOCRValuesWithLineBreaks = "";

foreach($ocrStepMap as $index => $ocrStepValue) {
    if($index % 8 == 0){
        $sOCRValuesWithLineBreaks .= "\n\t";
    }

    $sOCRValuesWithLineBreaks .= $ocrStepValue . ", ";
}

$sOCRValuesWithLineBreaks = rtrim($sOCRValuesWithLineBreaks, ', ');

$cOCRString = "// clock frequencies lookup table\n\n\nstatic const PROGMEM uint16_t frq_lookup[$arraySize] = {\t" . $sOCRValuesWithLineBreaks . "\n};";

file_put_contents('../include/clock_frq_lookup.h', $cOCRString);

/**
 * Calculate all dac_values for every step of the ADC, based on the frequencies
 */

// C = 1nF = 0.000000001
// R = 200K = 200000
// Vout = -10V

$voltsPerDACStep = 4 / 4096;
$milliVoltsPerDACStep = 4000 / 4096;
$dacValues = [];

foreach($frequencyMap as $frequencyMapValue)
{
    $time = 1 / $frequencyMapValue;
    //$vInCompensated = round(((-(0.000000001 * 200000 * -10) / $time) / 3) / $voltsPerDACStep);
    $vInCompensated = round(((0.697151065 * $frequencyMapValue) + 40.82) / $milliVoltsPerDACStep);

    // max out at 4096
    $dacValues[] = $vInCompensated < 4096 ? $vInCompensated : 4095;
}

// do some fancy formatting
$sDACValuesWithLineBreaks = "";

foreach($dacValues as $index => $dacValue) {
    if($index % 8 == 0){
        $sDACValuesWithLineBreaks .= "\n\t";
    }

    $sDACValuesWithLineBreaks .= $dacValue . ", ";
}

$sDACValuesWithLineBreaks = rtrim($sDACValuesWithLineBreaks, ', ');

$cDacString = "// dac values lookup table\n\n\nstatic const PROGMEM uint16_t dac_lookup[$arraySize] = {\t" . $sDACValuesWithLineBreaks . "\n};";

file_put_contents('../include/dac_voltage_lookup.h', $cDacString);

?>