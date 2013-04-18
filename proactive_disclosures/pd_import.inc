<?php

/**
 * Generate upload form for import.
 */
function pdisclosures_import_form(&$form_state) {  
  drupal_set_title(t("Proactive Disclosure Import"));
   
  $form['#attributes'] = array("enctype" => "multipart/form-data");
  
  // this is code for when we support ALL PD types but we only support Contracts for Phase 1
  /*$types = content_types();
  foreach($types as $name => $type) {
    if (substr($name, 0, 2) != "pd") continue;
    $pdtypes[$name] = str_replace('PD ', "", $type['name']);
  } */
  $pdtypes['pdcontract'] = t("Contract");
  
  $form['type'] = array(
    '#type' => 'select',
    '#title' => t('Select which Disclosure type you are importing'),
    '#options' => $pdtypes,
  );
  
  $form['import'] = array(
    '#type' => 'upload_element',
    '#title' => t('Import File'),
    '#required' => FALSE,
    /*'#file_validators' => array(
      'file_validate_size' => array(16384),
      'file_validate_extensions' => array('txt gif patch diff jpg jpeg'),
    ),*/
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  
  return $form;
}


/**
* Handle file import
* 
*   NOTE: we do not need to actually upload the file to the /files dir we simply need to process the file
*   so leaving it in tmp folder is sufficient
* 
* @param mixed $form
* @param mixed $form_state
* 
*   @todo:    this should be smarter, we should use header row to determine which fields and then use this based on type; but since we are only doing
*             Contracts for Phase 1 lets not bother.
* 
* Contract Fields:  OLD FORMAT
*   vendor_name
*   commitment_number
*   commitment_date
*   eo_code
*   descriptor_en
*   descriptor_fr
*   start_date
*   end_date
*   is_service_contract
*   commitment_amount
*   dept_name_number_en
*   dept_name_number_fr
*   comments_en
*   comments_fr
* 
* Contract Fields:  NEW FORMAT
*   vendor_name  
*   commitment_number  
*   commitment_date  
*   effective_date  
*   is_amendment  
*   eo_code  
*   descriptor_en
*   descriptor_fr  
*   start_date  
*   end_date  
*   is_service_contract  
*   commitment_amount  
*   commitment_amount_original  
*   dept_name_number_en  
*   dept_name_number_fr  
*   comments_en  
*   comments_fr
* 
*/
function pdisclosures_import_form_submit($form, &$form_state) {
  if($form_state['values']['import']->submit_action) {
    
    // get last batch number
    $batch = variable_get('pdisclosures_batch_number', 0) + 1;
    
    // load file into an array
    $file = file($form_state['values']['import']->filepath);
    
    // get rid of header row
    array_shift($file);
    
    // loop through each record and create new nodes
    foreach ($file as $record) {
      // parse tab delimited record
      $row = split("\t", $record);
      
      // set up std node values - EN first
      $node = new stdClass();  
      $node->type = $form_state['values']['type'];
      $node->uid = $GLOBALS['user']->uid;
      $node->language = 'en';
      
      // and add field values
      $node->field_vendor[0]['value'] = utf8_encode($row[0]);    
      $node->field_referencenumber[0]['value'] = $row[1];
      $node->field_contractdate[0]['value'] = $row[2];
      $node->field_effectivedate[0]['value'] = $row[3];
      $node->field_amendment[0]['value'] = $row[4];
      $node->field_economicobject[0]['value'] = $row[5]; 
      
      // Description
      $node->body = utf8_encode($row[6]);
      
      $node->field_startdate[0]['value'] = $row[8];     
      $node->field_enddate[0]['value'] = $row[9];   
      $node->field_servicecontract[0]['value'] = $row[10];
 
      // Contract Values - using new Money field type
      $node->field_contractvalue[0]['amount'] = $row[11];
        $node->field_contractvalue[0]['currency'] = t('CAD');
      $node->field_originalcontractvalue[0]['amount'] = $row[12];
        $node->field_originalcontractvalue[0]['currency'] = t('CAD');
        
      $node->field_comments[0]['value'] = utf8_encode($row[15]);
      
      // add batch number
      $node->field_pdbatch[0]['value'] = $batch;
      
      // create EN node
      node_save($node);
            
      // once we have a nid we need to set the tnid to this or it won't recognize the FR node as its translated node
      $node->tnid = $node->nid;
      node_save($node); 
      pathauto_node_update_alias($node, 'update'); 
      
      // then create FR node
      unset($node->nid, $node->vid);
      $node->language = 'fr';
      $node->body = $row[7];
      $node->field_comments[0]['value'] = $row[16];
      node_save($node);     
      pathauto_node_update_alias($node, 'update');  
    }
    variable_set('pdisclosures_batch_number', $batch); 
    drupal_set_message(format_plural(count($file),
      '1 record was imported successfully as batch #%batch.',  
      '@count records were imported successfully as batch #%batch.',
      array('%batch' => $batch)));
  }
}
 